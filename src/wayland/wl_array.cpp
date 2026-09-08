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

#include <mir/wayland/wl_array.h>

#include <boost/throw_exception.hpp>
#include <new>
#include <stdexcept>
#include <utility>

namespace mw = mir::wayland;

auto mw::WlArrayBase::operator=(WlArrayBase const& other) -> WlArrayBase&
{
    WlArrayBase temp{other};
    std::swap(array, temp.array);
    return *this;
}

auto mw::WlArrayBase::operator=(WlArrayBase&& other) noexcept -> WlArrayBase&
{
    if (this != &other)
    {
        wl_array_release(&array);
        array = *static_cast<wl_array*>(other);
        wl_array_init(other);
    }
    return *this;
}

auto mw::WlArrayBase::grow(size_t len) -> void*
{
    void* dest = wl_array_add(&array, len);
    if (!dest)
        BOOST_THROW_EXCEPTION(std::bad_alloc());
    return dest;
}

void mw::WlArrayBase::copy(wl_array* dst, wl_array const* src)
{
    // Const-cast is required because wl_array_copy takes a non-const pointer
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    if (wl_array_copy(dst, const_cast<wl_array*>(src)) < 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to copy wl_array"));
    }
}
