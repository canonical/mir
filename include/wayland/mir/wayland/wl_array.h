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
#include <algorithm>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include <concepts>

namespace mir
{
namespace wayland
{
template<typename T>
concept WlArrayElement = std::is_trivially_copyable_v<T>;
/// Owning RAII wrapper around a wl_array
template<WlArrayElement T>
class WlArray
{
public:
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init) - wl_array_init() initializes `array`
    WlArray() noexcept { wl_array_init(&array); }

    explicit WlArray(wl_array const* to_copy) : WlArray{} { copy(&array, to_copy); }

    WlArray(WlArray const& other) : WlArray{} { copy(&array, other); }

    WlArray(WlArray&& other) noexcept : array{*static_cast<wl_array*>(other)} { wl_array_init(other); }

    WlArray(T const* bytes, size_t len) : WlArray{} { append(bytes, bytes + len); }

    WlArray(std::initializer_list<T> init) : WlArray(init.begin(), init.size()) {}

    WlArray& operator=(WlArray const& other)
    {
        WlArray temp{other};
        std::swap(array, temp.array);
        return *this;
    }

    WlArray& operator=(WlArray&& other) noexcept
    {
        if (this != &other)
        {
            wl_array_release(&array);
            array = *static_cast<wl_array*>(other);
            wl_array_init(other);
        }
        return *this;
    }

    ~WlArray() { wl_array_release(&array); }

    void push_back(T const& value) { append(&value, &value + 1); }

    template<typename Iterator>
    void append(Iterator begin, Iterator end)
    {
        auto const count = std::distance(begin, end);
        if (count == 0)
            return;
        auto* dest = static_cast<T*>(wl_array_add(&array, static_cast<size_t>(count) * sizeof(T)));
        if (!dest)
            BOOST_THROW_EXCEPTION(std::bad_alloc());
        std::copy(begin, end, dest);
    }

    operator wl_array*() { return &array; }
    operator wl_array const*() const { return &array; }

private:
    wl_array array;

    static void copy(wl_array* dst, wl_array const* src)
    {
        // Const-cast is required because wl_array_copy takes a non-const pointer
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        if (wl_array_copy(dst, const_cast<wl_array*>(src)) < 0)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to copy wl_array"));
        }
    }
};
}
}

#endif // MIR_WAYLAND_WL_ARRAY_H_
