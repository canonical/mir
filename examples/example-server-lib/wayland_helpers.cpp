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

#include <mir/fd.h>
#include <boost/throw_exception.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <cstring>
#include <system_error>

struct wl_shm_pool* make_shm_pool(struct wl_shm* shm, int size, void **data)
{
    static auto (*open_shm_file)() -> mir::Fd = []
        {
            static char const* shm_dir;
            open_shm_file = []{ return mir::Fd{open(shm_dir, O_TMPFILE | O_RDWR | O_EXCL, S_IRWXU)}; };

            // Wayland based toolkits typically use $XDG_RUNTIME_DIR to open shm pools
            // so we try that before "/dev/shm". But confined snaps can't access "/dev/shm"
            // so we try "/tmp" if both of the above fail.
            for (auto dir : {const_cast<const char*>(getenv("XDG_RUNTIME_DIR")), "/dev/shm", "/tmp" })
            {
                if (dir)
                {
                    shm_dir = dir;
                    auto fd = open_shm_file();
                    if (fd >= 0)
                        return fd;
                }
            }

            // Workaround for filesystems that don't support O_TMPFILE (E.g. phones based on 3.4 or 3.10
            open_shm_file = []
            {
                char template_filename[] = "/dev/shm/mir-buffer-XXXXXX";
                mir::Fd fd{mkostemp(template_filename, O_CLOEXEC)};
                if (fd != -1)
                {
                    if (unlink(template_filename) < 0)
                    {
                        return mir::Fd{};
                    }
                }
                return fd;
            };

            return open_shm_file();
        };

    auto fd = open_shm_file();

    if (fd < 0) {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to open shm buffer"}));
    }

    if (auto error = posix_fallocate(fd, 0, size))
    {
        BOOST_THROW_EXCEPTION((std::system_error{error, std::system_category(), "Failed to allocate shm buffer"}));
    }

    if ((*data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to mmap buffer"}));
    }

    return wl_shm_create_pool(shm, fd, size);
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
    int32_t /*x*/,
    int32_t /*y*/,
    int32_t /*physical_width*/,
    int32_t /*physical_height*/,
    int32_t /*subpixel*/,
    const char */*make*/,
    const char */*model*/,
    int32_t transform)
{
    auto output = static_cast<Output*>(data);

    output->transform = transform;
}

void output_mode(
    void *data,
    struct wl_output* /*wl_output*/,
    uint32_t /*flags*/,
    int32_t /*width*/,
    int32_t /*height*/,
    int32_t /*refresh*/)
{
    auto output = static_cast<Output*>(data);
    (void)output;
}

void output_scale(void* data, struct wl_output* /*wl_output*/, int32_t factor)
{
    auto output = static_cast<Output*>(data);
    output->scale = factor;
}

}

wl_output_listener const Output::output_listener = {
    &output_geometry,
    &output_mode,
    &Output::output_done,
    &output_scale,
    nullptr,
    nullptr,
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
        wl_output_release(output);
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
        // NOTE: We'd normally need to do std::min(version, 3), lest the compositor only support version 1
        // of the interface. However, we're an internal client of a compositor that supports version 3, so…
        auto output =
            static_cast<wl_output*>(wl_registry_bind(registry, id, &wl_output_interface, 3));
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
