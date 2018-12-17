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

void Output::output_done(void* data, struct wl_output* /*wl_output*/)
{
    auto output = static_cast<Output*>(data);
    if (output->on_constructed)
    {
        output->on_constructed(*output);
        output->on_constructed = nullptr;
    }
    else
    {
        output->on_change(*output);
    }
}

namespace
{
void output_geometry(
    void* data,
    struct wl_output* /*wl_output*/,
    int32_t x,
    int32_t y,
    int32_t /*physical_width*/,
    int32_t /*physical_height*/,
    int32_t /*subpixel*/,
    const char */*make*/,
    const char */*model*/,
    int32_t transform)
{
    auto output = static_cast<Output*>(data);

    output->x = x;
    output->y = y;
    output->transform = transform;
}


void output_mode(
    void *data,
    struct wl_output* /*wl_output*/,
    uint32_t flags,
    int32_t width,
    int32_t height,
    int32_t /*refresh*/)
{
    if (!(WL_OUTPUT_MODE_CURRENT & flags))
        return;

    auto output = static_cast<Output*>(data);

    output->width = width,
    output->height = height;
}

void output_scale(void* data, struct wl_output* wl_output, int32_t factor)
{
    (void)data;
    (void)wl_output;
    (void)factor;
}

}

wl_output_listener const Output::output_listener = {
    &output_geometry,
    &output_mode,
    &Output::output_done,
    &output_scale,
};

Output::Output(
    wl_output* output,
    std::function<void(Output const&)> on_constructed,
    std::function<void(Output const&)> on_change)
    : output{output},
      on_constructed{std::move(on_constructed)},
      on_change{std::move(on_change)}
{
    wl_output_add_listener(output, &output_listener, this);
}

Output::~Output()
{
    if (output)
        wl_output_destroy(output);
}

Globals::Globals(
    std::function<void(Output const&)> on_new_output,
    std::function<void(Output const&)> on_output_changed,
    std::function<void(Output const&)> on_output_gone)
    : registry{nullptr, [](auto){}},
      on_new_output{std::move(on_new_output)},
      on_output_changed{std::move(on_output_changed)},
      on_output_gone{std::move(on_output_gone)}
{
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
        // NOTE: We'd normally need to do std::min(version, 2), lest the compositor only support version 1
        // of the interface. However, we're an internal client of a compositor that supports version 2, so…
        auto output =
            static_cast<wl_output*>(wl_registry_bind(registry, id, &wl_output_interface, 2));
        self->bound_outputs.insert(
            std::make_pair(
                id,
                std::make_unique<Output>(output, self->on_new_output, self->on_output_changed)));
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        self->shell = static_cast<decltype(self->shell)>(wl_registry_bind(registry, id, &wl_shell_interface, 1));
    }
}

void Globals::global_remove(
    void* data,
    struct wl_registry* registry,
    uint32_t id)
{
    (void)registry;
    Globals* self = static_cast<decltype(self)>(data);

    auto const output = self->bound_outputs.find(id);
    if (output != self->bound_outputs.end())
    {
        self->on_output_gone(*output->second);
        self->bound_outputs.erase(output);
    }
    // TODO: We should probably also delete any other globals we've bound to that disappear.
}

void Globals::init(struct wl_display* display)
{
    registry = {wl_display_get_registry(display), &wl_registry_destroy};

    wl_registry_add_listener(registry.get(), &registry_listener, this);
    wl_display_roundtrip(display);
    wl_display_roundtrip(display);
}

void Globals::teardown()
{
    bound_outputs.clear();
    registry.reset();
}
