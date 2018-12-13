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

#include "wayland_helpers.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <cstring>

struct wl_shm_pool* make_shm_pool(struct wl_shm* shm, int size, void **data)
{
    struct wl_shm_pool *pool;
    int fd;

    fd = open("/dev/shm", O_TMPFILE | O_RDWR | O_EXCL, S_IRWXU);
    if (fd < 0) {
        return NULL;
    }

    posix_fallocate(fd, 0, size);

    *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*data == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    pool = wl_shm_create_pool(shm, fd, size);

    close(fd);

    return pool;
}

void Globals::new_global(
    void* data,
    struct wl_registry* registry,
    uint32_t id,
    char const* interface,
    uint32_t version)
{
    (void)version;
    Globals* self = static_cast<decltype(self)>(data);

    if (strcmp(interface, "wl_compositor") == 0)
    {
        self->compositor =
            static_cast<decltype(self->compositor)>(wl_registry_bind(registry, id, &wl_compositor_interface, 3));
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        self->shm = static_cast<decltype(self->shm)>(wl_registry_bind(registry, id, &wl_shm_interface, 1));
        // Normally we'd add a listener to pick up the supported formats here
        // As luck would have it, I know that argb8888 is the only format we support :)
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        self->seat = static_cast<decltype(self->seat)>(wl_registry_bind(registry, id, &wl_seat_interface, 4));
    }
    else if (strcmp(interface, "wl_output") == 0)
    {
        self->output = static_cast<decltype(self->output)>(wl_registry_bind(registry, id, &wl_output_interface, 2));
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        self->shell = static_cast<decltype(self->shell)>(wl_registry_bind(registry, id, &wl_shell_interface, 1));
    }
}

void Globals::global_remove(
    void* data,
    struct wl_registry* registry,
    uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

void Globals::init(struct wl_display* display)
{
    wl_registry_listener const registry_listener = {
        new_global,
        global_remove
    };

    auto const registry = make_scoped(wl_display_get_registry(display), &wl_registry_destroy);

    wl_registry_add_listener(registry.get(), &registry_listener, this);
    wl_display_roundtrip(display);
}
