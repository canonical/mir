#include "mir/fd.h"
#include "mir/renderer/sw/pixel_source.h"

#include <span>
#include <sys/mman.h>

namespace mir
{
template<typename T>
class Mapping
{
public:
    virtual ~Mapping() = default;

    virtual T* data() = 0;
    virtual size_t len() const = 0;
};

class ReadMappableRange
{
public:
    virtual ~ReadMappableRange() = default;

    virtual auto map_ro() -> std::unique_ptr<Mapping<unsigned char const>> = 0;
};

class WriteMappableRange
{
public:
    virtual ~WriteMappableRange() = default;

    virtual auto map_wo() -> std::unique_ptr<Mapping<unsigned char>> = 0;
};

class RWMappableRange : public ReadMappableRange, public WriteMappableRange
{
public:
    virtual ~RWMappableRange() = default;

    virtual auto map_rw() -> std::unique_ptr<Mapping<unsigned char>> = 0;
};

namespace shm
{
template<int prot>
class Backing;

template<int prot>
auto make_shm_backing_store(mir::Fd const& backing, size_t claimed_size)
    -> std::shared_ptr<Backing<prot>>;

template<int prot, typename = std::enable_if_t<(prot & PROT_READ) && (prot & PROT_WRITE)>>
auto get_rw_range(std::shared_ptr<Backing<prot>> pool, size_t start, size_t len)
    -> std::unique_ptr<RWMappableRange>;

template<int prot, typename = std::enable_if_t<(prot & PROT_READ)>>
auto get_ro_range(std::shared_ptr<Backing<prot>> pool, size_t start, size_t len)
    -> std::unique_ptr<ReadMappableRange>;

template<int prot, typename = std::enable_if_t<(prot & PROT_WRITE)>>
auto get_wo_range(std::shared_ptr<Backing<prot>> pool, size_t start, size_t len)
    -> std::unique_ptr<WriteMappableRange>;
}
}
