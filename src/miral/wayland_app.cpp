/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wayland_app.h"
#include <cstring>
#include <algorithm>

wl_registry_listener const miral::tk::WaylandApp::registry_listener = {
    handle_new_global,
    handle_global_remove
};

miral::tk::WaylandApp::WaylandApp()
    : display_{nullptr, [](auto){ return 0; }}
{
}

void miral::tk::WaylandApp::wayland_init(wl_display* display)
{
    display_ = {display, wl_display_roundtrip};
    registry_ = {wl_display_get_registry(display), wl_registry_destroy};

    wl_registry_add_listener(registry_, &registry_listener, this);

    // Populate globals
    roundtrip();

    // Populate properties for outputs and other globals created in the first pass
    roundtrip();
}

void miral::tk::WaylandApp::roundtrip() const
{
    wl_display_roundtrip(display());
}

void miral::tk::WaylandApp::handle_new_global(
    void* data,
    struct wl_registry* registry,
    uint32_t id,
    char const* interface,
    uint32_t version)
{
    auto const self = static_cast<WaylandApp*>(data);

    if (strcmp(interface, "wl_compositor") == 0)
    {
        self->compositor_ = {
            static_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, 3)),
            wl_compositor_destroy
        };
        self->global_remove_handlers[id] = [self]() { self->compositor_ = {}; };
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        self->shm_ = {
            static_cast<wl_shm*>(wl_registry_bind(registry, id, &wl_shm_interface, 1)),
            wl_shm_destroy};
        self->global_remove_handlers[id] = [self](){ self->shm_ = {}; };
        // Normally we'd add a listener to pick up the supported formats here
        // As luck would have it, I know that argb8888 is the only format we support :)
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        self->seat_ = {
            static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, 4)),
            wl_seat_destroy};
        self->global_remove_handlers[id] = [self](){ self->seat_ = {}; };
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        self->shell_ = {
            static_cast<wl_shell*>(wl_registry_bind(registry, id, &wl_shell_interface, 1)),
            wl_shell_destroy};
        self->global_remove_handlers[id] = [self](){ self->shell_ = {}; };
    }
    else if (strcmp(interface, "zwlr_layer_shell_v1") == 0)
    {
        self->layer_shell_ = {
            static_cast<zwlr_layer_shell_v1*>(wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 4)),
            zwlr_layer_shell_v1_destroy
        };
        self->global_remove_handlers[id] = [self](){ self->layer_shell_ = {}; };
    }

    self->new_global(registry, id, interface, version);
}

void miral::tk::WaylandApp::handle_global_remove(void* data, struct wl_registry* /*registry*/, uint32_t id)
{
    auto const self = static_cast<WaylandApp*>(data);
    auto const handler = self->global_remove_handlers.find(id);
    if (handler != self->global_remove_handlers.end())
    {
        handler->second();
        self->global_remove_handlers.erase(handler);
    }
}
