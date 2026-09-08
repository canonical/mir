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

#include <algorithm>
#include <concepts>
#include <iterator>
#include <type_traits>

namespace mir
{
namespace wayland
{
/// Non-templated RAII owner of a raw wl_array: construction, destruction, copying and moving of
/// the underlying wl_array. Has no notion of element type
class WlArrayBase
{
public:
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init) - wl_array_init() initializes `array`
    WlArrayBase() noexcept { wl_array_init(&array); }

    explicit WlArrayBase(wl_array const* to_copy) : WlArrayBase{} { copy(&array, to_copy); }

    WlArrayBase(WlArrayBase const& other) : WlArrayBase{} { copy(&array, other); }

    WlArrayBase(WlArrayBase&& other) noexcept : array{*static_cast<wl_array*>(other)} { wl_array_init(other); }

    auto operator=(WlArrayBase const& other) -> WlArrayBase&;
    auto operator=(WlArrayBase&& other) noexcept -> WlArrayBase&;

    ~WlArrayBase() { wl_array_release(&array); }

    /// Grows the backing storage by `len` bytes and returns a pointer to the new space
    auto grow(size_t len) -> void*;

    operator wl_array*() { return &array; }
    operator wl_array const*() const { return &array; }

private:
    wl_array array;

    static void copy(wl_array* dst, wl_array const* src);
};

template<typename ValueType>
concept WlArrayElement = std::is_trivially_copyable_v<ValueType>;

/// Owning RAII wrapper around a wl_array
template<WlArrayElement ValueType>
class WlArray : WlArrayBase
{
public:
    using WlArrayBase::WlArrayBase;

    template<std::forward_iterator Iterator>
        requires std::same_as<std::iter_value_t<Iterator>, ValueType>
    WlArray(Iterator begin, Iterator end) : WlArray{}
    {
        append(begin, end);
    }

    WlArray(std::initializer_list<ValueType> init) : WlArray(init.begin(), init.end()) {}

    using WlArrayBase::operator=;

    void push_back(ValueType const& value) { append(&value, std::next(&value)); }

    /// Appends the elements in [begin, end), growing the backing storage as needed
    template<std::forward_iterator Iterator>
        requires std::same_as<std::iter_value_t<Iterator>, ValueType>
    void append(Iterator begin, Iterator end)
    {
        auto const count = std::distance(begin, end);
        auto* dest = static_cast<ValueType*>(grow(static_cast<size_t>(count) * sizeof(ValueType)));
        std::copy(begin, end, dest);
    }

    using WlArrayBase::operator wl_array*;
    using WlArrayBase::operator wl_array const*;
};
}
}

#endif // MIR_WAYLAND_WL_ARRAY_H_
