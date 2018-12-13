/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_WAYLAND_HELPERS_H
#define MIRAL_WAYLAND_HELPERS_H

#include <wayland-client.h>
#include <memory>

template<typename Type>
auto make_scoped(Type* owned, void(*deleter)(Type*)) -> std::unique_ptr<Type, void(*)(Type*)>
{
    return {owned, deleter};
}

wl_shm_pool* make_shm_pool(struct wl_shm* shm, int size, void **data);

struct Globals
{
    wl_compositor* compositor = nullptr;
    wl_shm* shm = nullptr;
    wl_seat* seat = nullptr;
    wl_output* output = nullptr;
    wl_shell* shell = nullptr;

    void init(struct wl_display* display);

private:
    static void new_global(
        void* data,
        struct wl_registry* registry,
        uint32_t id,
        char const* interface,
        uint32_t version);

    static void global_remove(
        void* data,
        struct wl_registry* registry,
        uint32_t name);
};

#endif //MIRAL_WAYLAND_HELPERS_H
