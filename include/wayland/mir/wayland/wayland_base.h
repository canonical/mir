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

#include <boost/throw_exception.hpp>

#include <memory>

struct wl_resource;
struct wl_global;
struct wl_client;

namespace mir
{
namespace wayland
{

class Resource
{
public:
    template<int V>
    struct Version
    {
    };

    Resource();
    virtual ~Resource();

    Resource(Resource const&) = delete;
    Resource& operator=(Resource const&) = delete;

    auto destroyed_flag() const -> std::shared_ptr<bool>;

private:
    std::shared_ptr<bool> mutable destroyed;
};

/// A weak handle to a Wayland resource
/// May only be safely used from the Wayland thread
template<typename T>
class Weak
{
public:
    Weak(T* resource)
        : resource{resource},
          destroyed_flag{resource->destroyed_flag()}
    {
    }

    Weak(Weak<T> const&) = default;
    auto operator=(Weak<T> const&) -> Weak<T>& = default;

    auto operator==(Weak<T> const& other) const -> bool
    {
        return resource == other->resource;
    }

    operator bool() const
    {
        return !*destroyed_flag;
    }

    auto value() const -> T&
    {
        if (*destroyed_flag)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("Attempted access of destroyed Wayland resource"));
        }
        return *resource;
    }

private:
    T* resource;
    std::shared_ptr<bool> destroyed_flag;
};

template<typename T>
auto make_weak(T* resource) -> Weak<T>
{
    return Weak<T>{resource};
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

    virtual auto interface_name() const -> char const* = 0;

    wl_global* const global;
};

void internal_error_processing_request(wl_client* client, char const* method_name);

}
}

#endif // MIR_WAYLAND_OBJECT_H_
