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

class RWShmBacking
{
public:
    RWShmBacking(mir::Fd const& backing_store, size_t claimed_size);

    static auto get_rw_range(std::shared_ptr<RWShmBacking> pool, size_t start, size_t len)
        -> std::unique_ptr<RWMappableRange>;

private:

    void* mapped_address;
    size_t size;
    bool size_is_trustworthy{false};
};
}
