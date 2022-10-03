#include "shm_backing.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <system_error>
#include <memory>
#include <boost/throw_exception.hpp>

namespace
{
class ShmBacking
{
public:
    ShmBacking(mir::Fd const& backing_store, size_t claimed_size, int prot);
    ~ShmBacking();

    template<typename Mapping, typename Parent>
    auto get_range(size_t start, size_t len, std::shared_ptr<Parent> parent)
        -> std::unique_ptr<Mapping>;
private:
    void* mapped_address;
    size_t size;
    bool size_is_trustworthy{false};
};

ShmBacking::ShmBacking(mir::Fd const& backing_store, size_t claimed_size, int prot)
    : size{claimed_size}
{
    mapped_address = mmap(nullptr, claimed_size, prot, MAP_SHARED, backing_store, 0);
    if (mapped_address == MAP_FAILED)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            errno,
            std::system_category(),
            "Failed to map client-provided SHM pool"}));
    }

    int file_seals = fcntl(backing_store, F_GET_SEALS);
    if (file_seals == -1)
    {
        // Unsupported? Some other error? We don't really care, this is just an optimisation
        // If we can't do that optimisation, we're done.
        return;
    }
    struct stat file_info;
    if (file_seals & F_SEAL_SHRINK)
    {
        /* The kernel promises that the underlying file cannot get smaller, so we can
         * usefully check to see if it's big enough now, and if so, we don't have to do the SIGBUS
         * dance
         */
        if (fstat(backing_store, &file_info) >= 0)
        {
            size_is_trustworthy = static_cast<size_t>(file_info.st_size) >= claimed_size;
        }
    }
}

ShmBacking::~ShmBacking()
{
    munmap(mapped_address, size);
}

template<typename Mapping, typename Parent>
auto ShmBacking::get_range(size_t start, size_t len, std::shared_ptr<Parent> parent)
    -> std::unique_ptr<Mapping>
{
    // This slightly weird comparison is to avoid integer overflow
    if ((start > size) ||
        (size - start < len))
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Attempt to get a range outside the SHM backing"}));
    }
    auto start_addr = static_cast<char*>(mapped_address) + start;
    return std::make_unique<Mapping>(start_addr, len, std::move(parent));
}

template<typename T>
class ShmBackedMapping : public mir::Mapping<T>
{
public:
    ShmBackedMapping(T* data, size_t size)
        : ptr{data},
          size{size}
    {
    }

    auto data() -> T* override
    {
        return ptr;
    }

    auto len() const -> size_t override
    {
        return size;
    }
private:
    T* const ptr;
    size_t const size;
};

class ROMappableRange : public mir::ReadMappableRange
{
public:
    ROMappableRange(void* ptr, size_t len, std::shared_ptr<mir::shm::ReadOnlyPool> parent)
        : ptr{ptr},
          len{len},
          parent{std::move(parent)}
    {
    }

    auto map_ro() -> std::unique_ptr<mir::Mapping<std::byte const>> override
    {
        return std::make_unique<ShmBackedMapping<std::byte const>>(static_cast<std::byte const*>(ptr), len);
    }
private:
    void* const ptr;
    size_t const len;
    std::shared_ptr<mir::shm::ReadOnlyPool> const parent;
};

class WOMappableRange : public mir::WriteMappableRange
{
public:
    WOMappableRange(void* ptr, size_t len, std::shared_ptr<mir::shm::WriteOnlyPool> parent)
        : ptr{ptr},
          len{len},
          parent{std::move(parent)}
    {
    }

    auto map_wo() -> std::unique_ptr<mir::Mapping<std::byte>> override
    {
        return std::make_unique<ShmBackedMapping<std::byte>>(static_cast<std::byte*>(ptr), len);
    }
private:
    void* const ptr;
    size_t const len;
    std::shared_ptr<mir::shm::WriteOnlyPool> const parent;
};

class RWShmBackedPool;

class RWMappableRange : public mir::RWMappableRange
{
public:
    RWMappableRange(void* ptr, size_t len, std::shared_ptr<RWShmBackedPool> parent)
        : ptr{ptr},
          len{len},
          parent{std::move(parent)}
    {
    }

    auto map_rw() -> std::unique_ptr<mir::Mapping<std::byte>> override
    {
        return std::make_unique<ShmBackedMapping<std::byte>>(static_cast<std::byte*>(ptr), len);
    }

    auto map_ro() -> std::unique_ptr<mir::Mapping<std::byte const>> override
    {
        return std::make_unique<ShmBackedMapping<std::byte const>>(static_cast<std::byte*>(ptr), len);
    }

    auto map_wo() -> std::unique_ptr<mir::Mapping<std::byte>> override
    {
        return std::make_unique<ShmBackedMapping<std::byte>>(static_cast<std::byte*>(ptr), len);
    }
private:
    void* const ptr;
    size_t const len;
    std::shared_ptr<RWShmBackedPool> const parent;
};

class RWShmBackedPool : public mir::shm::ReadWritePool, public std::enable_shared_from_this<RWShmBackedPool>
{
public:
    RWShmBackedPool(mir::Fd const& backing, size_t claimed_size)
        : backing_store{backing, claimed_size, PROT_READ | PROT_WRITE}
    {
    }

    auto get_rw_range(size_t start, size_t len) -> std::unique_ptr<mir::RWMappableRange> override
    {
        return backing_store.get_range<RWMappableRange>(start, len, shared_from_this());
    }

    auto get_ro_range(size_t start, size_t len) -> std::unique_ptr<mir::ReadMappableRange> override
    {
        return backing_store.get_range<ROMappableRange>(start, len, shared_from_this());
    }

    auto get_wo_range(size_t start, size_t len) -> std::unique_ptr<mir::WriteMappableRange> override
    {
        return backing_store.get_range<WOMappableRange>(start, len, shared_from_this());
    }
private:
    ShmBacking backing_store; 
};
}

auto mir::shm::rw_pool_from_fd(mir::Fd const& backing, size_t claimed_size) -> std::shared_ptr<ReadWritePool>
{
    return std::make_shared<RWShmBackedPool>(backing, claimed_size);
}
