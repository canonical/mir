#include "shm_backing.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <system_error>
#include <boost/throw_exception.hpp>

mir::RWShmBacking::RWShmBacking(mir::Fd const& backing_store, size_t claimed_size)
    : size{claimed_size}
{
    mapped_address = mmap(nullptr, claimed_size, PROT_READ | PROT_WRITE, MAP_SHARED, backing_store, 0);
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

namespace
{
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

auto mir::RWShmBacking::get_rw_range(std::shared_ptr<RWShmBacking> pool, size_t start, size_t len)
    -> std::unique_ptr<RWMappableRange>
{
    auto start_addr = static_cast<char*>(pool->mapped_address) + start;
    return std::make_unique<::RWMappableRange>(start_addr, len);
}
