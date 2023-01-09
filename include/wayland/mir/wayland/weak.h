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

#ifndef MIR_WAYLAND_WEAK_H_
#define MIR_WAYLAND_WEAK_H_

#include <memory>
#include <boost/throw_exception.hpp>

namespace mir
{
namespace wayland
{
/// A weak handle to a Wayland resource (or any Destroyable)
/// May only be safely used from the Wayland thread
template<typename T>
class Weak
{
public:
    Weak()
        : resource{nullptr},
          destroyed_flag{nullptr}
    {
    }

    explicit Weak(T* resource)
        : resource{resource},
          destroyed_flag{resource ? resource->destroyed_flag() : nullptr}
    {
    }

    Weak(Weak<T> const&) = default;
    auto operator=(Weak<T> const&) -> Weak<T>& = default;

    auto operator==(Weak<T> const& other) const -> bool
    {
        if (*this && other)
        {
            return resource == other.resource;
        }
        else
        {
            return (!*this && !other);
        }
    }

    auto operator!=(Weak<T> const& other) const -> bool
    {
        return !(*this == other);
    }

    auto is(T const& other) const -> bool
    {
        if (*this)
        {
            return resource == &other;
        }
        else
        {
            return false;
        }
    }

    operator bool() const
    {
        return resource && !*destroyed_flag;
    }

    auto value() const -> T&
    {
        if (!*this)
        {
            BOOST_THROW_EXCEPTION(std::logic_error(
                std::string{"Attempted access of "} +
                (resource ? "destroyed" : "null") +
                " wayland::Weak<" + typeid(T).name() + ">"));
        }
        return *resource;
    }

private:
    T* resource;
    /// Is null if and only if resource is null
    /// If the target bool is true then resource has been freed and should not be used
    std::shared_ptr<bool const> destroyed_flag;
};

template<typename T>
auto make_weak(T* resource) -> Weak<T>
{
    return Weak<T>{resource};
}

template<typename T>
auto as_nullable_ptr(Weak<T> const& weak) -> T*
{
    return weak ? &weak.value() : nullptr;
}
}
}

#endif // MIR_WAYLAND_WEAK_H_
