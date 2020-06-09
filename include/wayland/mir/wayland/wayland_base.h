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
    ~Resource();

    Resource(Resource const&) = delete;
    Resource& operator=(Resource const&) = delete;

    auto destroyed_flag() const -> std::shared_ptr<bool>;

private:
    std::shared_ptr<bool> mutable destroyed;
};

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
