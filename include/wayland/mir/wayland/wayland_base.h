/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_WAYLAND_OBJECT_H_
#define MIR_WAYLAND_OBJECT_H_

#include "mir/int_wrapper.h"

#include <boost/throw_exception.hpp>
#include <memory>
#include <functional>
#include <stdexcept>

struct wl_resource;
struct wl_global;
struct wl_client;

namespace mir
{
namespace wayland
{
/// For when the protocol does not provide an appropriate error code
uint32_t const generic_error_code = -1;

/**
 * An exception type representing a Wayland protocol error
 *
 * Throwing one of these from a request handler will result in the client
 * being sent a \a code error on \a source, with the printf-style \a fmt string
 * populated as the message.:
 */
class ProtocolError : public std::runtime_error
{
public:
    [[gnu::format (printf, 4, 5)]]  // Format attribute counts the hidden this parameter
    ProtocolError(wl_resource* source, uint32_t code, char const* fmt, ...);

    auto message() const -> char const*;
    auto resource() const -> wl_resource*;
    auto code() const -> uint32_t;
private:
    std::string error_message;
    wl_resource* const source;
    uint32_t const error_code;
};

namespace detail
{
struct DestroyListenerIdTag;
}

typedef IntWrapper<detail::DestroyListenerIdTag> DestroyListenerId;

/// The base class of any object that wants to provide a destroyed flag
/// The destroyed flag is only created when needed and automatically set to true on destruction
/// This pattern is only safe in a single-threaded context
class LifetimeTracker
{
public:
    LifetimeTracker();
    LifetimeTracker(LifetimeTracker const&) = delete;
    LifetimeTracker& operator=(LifetimeTracker const&) = delete;

    virtual ~LifetimeTracker();
    /// The pointed-at bool contains false if this object is still alive and true if it has been destroyed.
    auto destroyed_flag() const -> std::shared_ptr<bool const>;
    /// The given function will be called just before the object is marked as destroyed. The returned ID can be used
    /// to remove the listener in which case it is never called. DestroyListenerId{} (value 0) is never returned, and so
    /// it can be used as a null ID. Destroy listener call order is undefined.
    auto add_destroy_listener(std::function<void()> listener) const -> DestroyListenerId;
    /// If the given ID maps to a destroy listener, that listener is dropped without being called. If the listener has
    /// already been dropped or never existed, this call is ignored.
    void remove_destroy_listener(DestroyListenerId id) const;

protected:
    /// Subclasses are not required to call this, but may do so during the destruction process if the object needs to
    /// get marked as destroyed and fire its destroy listeners before some other part of the destructor runs.
    void mark_destroyed() const;

private:
    struct Impl;

    /// Since many Wayland objects are created and the features of this class are used for only a few, impl is created
    /// lazily to conserve memory.
    std::unique_ptr<Impl> mutable impl;
};

class Resource
    : public virtual LifetimeTracker
{
public:
    template<int V>
    struct Version
    {
    };

    Resource();
};

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

class Global
{
public:
    template<int V>
    struct Version
    {
    };

    explicit Global(wl_global* global);
    virtual ~Global();

    Global(Global const&) = delete;
    Global& operator=(Global const&) = delete;

    auto interface_name() const -> char const*;

private:
    wl_global* const global;
};

void internal_error_processing_request(wl_client* client, char const* method_name);

}
}

#endif // MIR_WAYLAND_OBJECT_H_
