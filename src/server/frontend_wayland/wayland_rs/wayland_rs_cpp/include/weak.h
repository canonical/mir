/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIR_WAYLANDRS_WEAK
#define MIR_WAYLANDRS_WEAK

#include <memory>
#include <stdexcept>
#include <utility>
#include <boost/throw_exception.hpp>

namespace mir
{
namespace wayland_rs
{
template<typename T>
class Weak
{
public:
    Weak() = default;

    explicit Weak(std::shared_ptr<T> const& ptr)
        : ptr_{ptr}
    {
    }

    Weak(Weak const& weak) = default;

    T& value() const
    {
        return *ptr_.lock();
    }

    operator bool() const
    {
        return !ptr_.expired();
    }

    auto is(T const& other) const -> bool
    {
        auto locked = ptr_.lock();
        return locked && locked.get() == &other;
    }

    auto operator==(Weak<T> const& other) const -> bool
    {
        return !ptr_.owner_before(other.ptr_) && !other.ptr_.owner_before(ptr_);
    }

    auto operator!=(Weak<T> const& other) const -> bool
    {
        return !(*this == other);
    }

    auto operator=(Weak<T> const&) -> Weak<T>& = default;

private:
    std::weak_ptr<T> ptr_;
};
}
}

#endif  // MIR_WAYLANDRS_WEAK
