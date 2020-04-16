/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "cursor.h"
#include "displayclient.h"

#include <mir/fd.h>

#include <wayland-client.h>

#include <boost/throw_exception.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <cstring>
#include <system_error>

namespace mpw = mir::platform::wayland;

namespace
{
struct wl_shm_pool* make_shm_pool(struct wl_shm* shm, int size, void **data)
{
    // As we're a Wayland client, create the shm file like Wayland clients.
    // While using O_TMPFILE would be more elegant, this works with Snap-confined servers.
    static auto const template_filename =
        std::string{getenv("XDG_RUNTIME_DIR")} + "/wayland-cursor-shared-XXXXXX";

    auto const filename = strdup(template_filename.c_str());
    mir::Fd const fd{mkostemp(filename, O_CLOEXEC)};
    unlink(filename);
    free(filename);

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
}

mpw::Cursor::Cursor(wl_display* display, wl_compositor* compositor, wl_shm* shm) :
    display{display},
    shm{shm},
    surface{wl_compositor_create_surface(compositor)}
{
}

mpw::Cursor::~Cursor()
{
    wl_surface_destroy(surface);

    std::lock_guard<decltype(mutex)> lock{mutex};
    if (buffer) wl_buffer_destroy(buffer);
}

void mpw::Cursor::move_to(geometry::Point)
{
}

void mpw::Cursor::show(graphics::CursorImage const& cursor_image)
{
    {
        std::lock_guard<decltype(mutex)> lock{mutex};
        if (buffer) wl_buffer_destroy(buffer);

        auto const width = cursor_image.size().width.as_uint32_t();
        auto const height = cursor_image.size().height.as_uint32_t();
        auto const hotspot_x = cursor_image.hotspot().dx.as_uint32_t();
        auto const hotspot_y = cursor_image.hotspot().dy.as_uint32_t();
        void* data_buffer;
        auto const shm_pool = make_shm_pool(shm, 4 * width * height, &data_buffer);
        memcpy(data_buffer, cursor_image.as_argb_8888(), 4 * width * height);
        buffer = wl_shm_pool_create_buffer(shm_pool, 0, width, height, 4 * width, WL_SHM_FORMAT_ARGB8888);
        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_commit(surface);
        wl_shm_pool_destroy(shm_pool);
        if (pointer) wl_pointer_set_cursor(pointer, 0, surface, hotspot_x, hotspot_y);
    }
    wl_display_roundtrip(display);
}

void mpw::Cursor::hide()
{
}

void mir::platform::wayland::Cursor::enter(wl_pointer* pointer)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    this->pointer = pointer;
}

void mir::platform::wayland::Cursor::leave(wl_pointer* /*pointer*/)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    pointer = nullptr;
}
