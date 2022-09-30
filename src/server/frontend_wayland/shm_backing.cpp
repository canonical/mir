#include "shm_backing.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <system_error>
#include <boost/throw_exception.hpp>

namespace
{
class ShmBacking
{
public:
    ShmBacking(mir::Fd const& backing_store, size_t claimed_size, int prot);
    ~ShmBacking();

    template<typename Mapping>
    auto get_range(size_t start, size_t len)
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

template<typename Mapping>
auto ShmBacking::get_range(size_t start, size_t len)
    -> std::unique_ptr<Mapping>
{
    // This slightly weird comparison is to avoid integer overflow
    if ((start > size) ||
        (size - start < len))
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Attempt to get a range outside the SHM backing"}));
    }
    auto start_addr = static_cast<char*>(mapped_address) + start;
    return std::make_unique<Mapping>(start_addr, len);
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

class RWMappableRange : public mir::RWMappableRange
{
public:
    RWMappableRange(void* ptr, size_t len)
        : ptr{ptr},
          len{len}
    {
    }

    auto map_rw() -> std::unique_ptr<mir::Mapping<unsigned char>>
    {
        return std::make_unique<ShmBackedMapping<unsigned char>>(static_cast<unsigned char*>(ptr), len);
    }

    auto map_wo() -> std::unique_ptr<mir::Mapping<unsigned char>>
    {
        return std::make_unique<ShmBackedMapping<unsigned char>>(static_cast<unsigned char*>(ptr), len);
    }

    auto map_ro() -> std::unique_ptr<mir::Mapping<unsigned char const>>
    {
        return std::make_unique<ShmBackedMapping<unsigned char const>>(static_cast<unsigned char*>(ptr), len);
    }
private:
    void* const ptr;
    size_t const len;
};
}

template<int prot>
class mir::shm::Backing
{
public:
    Backing(mir::Fd const& backing, size_t claimed_size)
        : backing{backing, claimed_size, prot}
    {
    }

    ::ShmBacking backing;    
};

template<int prot>
auto mir::shm::make_shm_backing_store(mir::Fd const& backing, size_t claimed_size)
    -> std::shared_ptr<Backing<prot>>
{
    return std::make_shared<Backing<prot>>(backing, claimed_size);
}

template
auto mir::shm::make_shm_backing_store<PROT_READ>(mir::Fd const&, size_t)
  -> std::shared_ptr<Backing<PROT_READ>>;
template
auto mir::shm::make_shm_backing_store<PROT_WRITE>(mir::Fd const&, size_t)
  -> std::shared_ptr<Backing<PROT_WRITE>>;
template
auto mir::shm::make_shm_backing_store<PROT_WRITE | PROT_READ>(mir::Fd const&, size_t)
  -> std::shared_ptr<Backing<PROT_WRITE | PROT_READ>>;

template<>
auto mir::shm::get_rw_range(std::shared_ptr<mir::shm::Backing<PROT_READ | PROT_WRITE>> pool, size_t start, size_t len)
    -> std::unique_ptr<mir::RWMappableRange>
{
    return pool->backing.get_range<::RWMappableRange>(start, len);
}

