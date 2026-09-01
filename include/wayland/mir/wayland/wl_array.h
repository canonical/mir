/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_WAYLAND_WL_ARRAY_H_
#define MIR_WAYLAND_WL_ARRAY_H_

#include <wayland-util.h>

#include <boost/throw_exception.hpp>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

namespace mir
{
namespace wayland
{
/// Owning RAII wrapper around a wl_array
class WlArray
{
public:
    WlArray() noexcept { wl_array_init(&array); }

    explicit WlArray(wl_array const* to_copy)
    {
        wl_array_init(&array);
        if (wl_array_copy(&array, const_cast<wl_array*>(to_copy)) < 0)
            BOOST_THROW_EXCEPTION(std::bad_alloc());
    }

    WlArray(WlArray const& other) : WlArray{&other.array} {}

    WlArray(WlArray&& other) noexcept : array{other.array} { wl_array_init(&other.array); }

    WlArray& operator=(WlArray const& other)
    {
        if (this != &other)
        {
            wl_array_release(&array);
            wl_array_init(&array);

            if (wl_array_copy(&array, const_cast<wl_array*>(&other.array)) < 0)
            {
                BOOST_THROW_EXCEPTION(std::bad_alloc());
            }
        }

        return *this;
    }

    WlArray& operator=(WlArray&& other) noexcept
    {
        if (this != &other)
        {
            wl_array_release(&array);
            array = other.array;
            wl_array_init(&other.array);
        }
        return *this;
    }

    ~WlArray() { wl_array_release(&array); }

    /// Appends a trivially-copyable value, growing the backing storage as needed
    template<typename T>
    void push_back(T const& value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        append(&value, sizeof(T));
    }

    /// Appends raw bytes, growing the backing storage as needed
    void append(void const* bytes, size_t len)
    {
        if (!len)
            return;
        void* dest = wl_array_add(&array, len);
        if (!dest)
            BOOST_THROW_EXCEPTION(std::bad_alloc());
        std::memcpy(dest, bytes, len);
    }

    /// For passing into generated send_*_event()/request functions, which take
    /// a non-const wl_array* regardless of direction
    [[nodiscard]] auto data() const -> wl_array* { return const_cast<wl_array*>(&array); }

private:
    wl_array array;
};
}
}

#endif // MIR_WAYLAND_WL_ARRAY_H_
