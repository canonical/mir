/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/fd.h>
#include <mir/renderer/sw/pixel_source.h>

#include <cstddef>
#include <sys/mman.h>

namespace mir
{
namespace shm
{

/**
 * Interface for a directly-accessible mapped view of shared memory
 *
 * Mapping<T> provides a view much like `std::array<T>` onto some form of
 * shared memory backing.
 *
 * \tparam T    The type to interpret the underlying memory as
 */
template<typename T>
class Mapping
{
public:
    Mapping() = default;
    virtual ~Mapping() = default;

    Mapping(Mapping const&) = delete;
    auto operator=(Mapping const&) = delete;

    // Mapping<T> interface
    virtual T* data() = 0;
    virtual T const* data() const = 0;
    virtual size_t len() const = 0;

    /**
     * Check if access to this mapping has failed
     *
     * The underlying storage for a `mmap` region may not exist (for example,
     * `ftruncate` can shrink an existing `mmap`ed file), in which case accesses
     * to the missing region will generate a fault.
     *
     * Implementations of Mapping<T> will ensure that such access faults are
     * not fatal (although faulting reads will return 0 and faulting writes
     * will not be visible in any other mapping).
     *
     * \ref access_fault() returns whether or not an access fault has occured
     * in the lifetime of this Mapping<T>.
     *
     * \returns    Whether an access fault has occured on this Mapping<T>
     */
    virtual auto access_fault() const -> bool = 0;

    // Convenience functions for accessing a Mapping<T>
    auto operator[](size_t idx) -> T&
    {
        return data()[idx];
    }
    auto operator[](size_t idx) const -> T const&
    {
        return data()[idx];
    }

    auto begin() -> T*
    {
        return data();
    }
    auto end() -> T*
    {
        return data() + len();
    }

    auto begin() const -> T const*
    {
        return data();
    }
    auto end() const -> T const*
    {
        return data() + len();
    }

    auto cbegin() const -> T const*
    {
        return data();
    }
    auto cend() const -> T const*
    {
        return data() + len();
    }
};

class ReadMappableRange
{
public:
    virtual ~ReadMappableRange() = default;

    virtual auto map_ro() -> std::unique_ptr<Mapping<std::byte const>> = 0;
};

class WriteMappableRange
{
public:
    virtual ~WriteMappableRange() = default;

    virtual auto map_wo() -> std::unique_ptr<Mapping<std::byte>> = 0;
};

class RWMappableRange : public ReadMappableRange, public WriteMappableRange
{
public:
    virtual ~RWMappableRange() = default;

    virtual auto map_rw() -> std::unique_ptr<Mapping<std::byte>> = 0;
};

class ReadOnlyPool
{
public:
    ReadOnlyPool() = default;
    virtual ~ReadOnlyPool() = default;

    virtual auto get_ro_range(size_t start, size_t len) -> std::unique_ptr<ReadMappableRange> = 0;
    virtual void resize(size_t new_size) = 0;
};

class WriteOnlyPool
{
public:
    WriteOnlyPool() = default;
    virtual ~WriteOnlyPool() = default;

    virtual auto get_wo_range(size_t start, size_t len) -> std::unique_ptr<WriteMappableRange> = 0;
    virtual void resize(size_t new_size) = 0;
};

class ReadWritePool : public ReadOnlyPool, public WriteOnlyPool
{
public:
    ReadWritePool() = default;
    virtual ~ReadWritePool() = default;

    virtual auto get_rw_range(size_t start, size_t len) -> std::unique_ptr<RWMappableRange> = 0;
    void resize(size_t new_size) override = 0;
};

auto rw_pool_from_fd(mir::Fd backing, size_t claimed_size) -> std::shared_ptr<ReadWritePool>;
}
}
