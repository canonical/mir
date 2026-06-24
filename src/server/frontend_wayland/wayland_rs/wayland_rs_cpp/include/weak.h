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
#include <string>
#include <typeinfo>
#include <boost/throw_exception.hpp>

namespace mir
{
namespace wayland_rs
{
/// A weak handle to a Wayland object (any LifetimeTracker subclass).
///
/// Unlike a std::weak_ptr, a Weak does not require the pointed-at object to be owned by a
/// std::shared_ptr, and creating one never requires std::enable_shared_from_this: the object's
/// own destroyed-flag (provided by LifetimeTracker) is used as the control block. This means an
/// object can hand out a weak reference to itself with `make_weak(this)`.
///
/// May only be safely used from the Wayland thread.
template<typename T>
class Weak
{
public:
    Weak() : resource{nullptr}, destroyed_flag{nullptr} {}

    explicit Weak(T* resource)
        : resource{resource},
          destroyed_flag{resource ? resource->destroyed_flag() : nullptr}
    {
    }

    /// Convenience overload: the dispatch layer owns objects via std::shared_ptr. The Weak does
    /// not keep the object alive; it only borrows the raw pointer and the object's destroyed-flag.
    explicit Weak(std::shared_ptr<T> const& resource)
        : Weak{resource.get()}
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

    operator bool() const { return resource && !*destroyed_flag; }

    auto value() const -> T&
    {
        if (!*this)
        {
            BOOST_THROW_EXCEPTION(
                std::logic_error(
                    std::string{"Attempted access of "} + (resource ? "destroyed" : "null") +
                    " wayland_rs::Weak<" + typeid(T).name() + ">"));
        }
        return *resource;
    }

    /// Returns a Weak to the same object viewed as U (a base or derived class), or an empty Weak
    /// if this Weak is null/expired or the dynamic_cast fails. The destroyed-flag is preserved
    /// because the resulting Weak is re-derived from the very same (live) object.
    template<typename U>
    auto as() const -> Weak<U>
    {
        if (!*this)
        {
            return Weak<U>{};
        }
        return Weak<U>{dynamic_cast<U*>(resource)};
    }

private:
    T* resource;
    /// Is null if and only if resource is null.
    /// If the target bool is true then resource has been freed and should not be used.
    std::shared_ptr<bool const> destroyed_flag;
};

template<typename T>
auto make_weak(T* resource) -> Weak<T>
{ return Weak<T>{resource}; }

template<typename T>
auto make_weak(std::shared_ptr<T> const& resource) -> Weak<T>
{ return Weak<T>{resource}; }

template<typename T>
auto as_nullable_ptr(Weak<T> const& weak) -> T*
{ return weak ? &weak.value() : nullptr; }

/// Casts a Weak<T> to a Weak<U> between related (base/derived) types, preserving the
/// destroyed-flag. Yields an empty Weak<U> when the source is null/expired or the cast fails.
template<typename U, typename T>
auto weak_cast(Weak<T> const& weak) -> Weak<U>
{ return weak.template as<U>(); }
}
}

/// Declares a backwards-compatible `static Derived* from(Weak<Base> const&)` recovery method on a
/// concrete Wayland implementation, mirroring the legacy `Concrete::from(wl_resource*)` ergonomics.
/// Returns the concrete pointer, or nullptr if the Weak is null/expired or does not refer to a
/// Derived. Built on weak_cast<>/as_nullable_ptr.
///
/// Usage (inside the class body of Derived):
///     MIR_WAYLANDRS_DECLARE_FROM(WlSurfaceImpl, Surface)
#define MIR_WAYLANDRS_DECLARE_FROM(Derived, Base)                              \
    static auto from(::mir::wayland_rs::Weak<Base> const& weak) -> Derived*    \
    {                                                                          \
        return ::mir::wayland_rs::as_nullable_ptr(                             \
            ::mir::wayland_rs::weak_cast<Derived>(weak));                      \
    }

#endif  // MIR_WAYLANDRS_WEAK
