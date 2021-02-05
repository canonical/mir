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

#ifndef MIRAL_WAYLAND_SHM_H
#define MIRAL_WAYLAND_SHM_H

#include "mir/geometry/size.h"

#include <wayland-client.h>
#include <memory>

class WaylandShm;

// TODO: migrate away from using this directly
struct wl_shm_pool* make_shm_pool(struct wl_shm* shm, int size, void **data);

class WaylandShmBuffer : public std::enable_shared_from_this<WaylandShmBuffer>
{
public:
    WaylandShmBuffer(
        void* data,
        size_t data_size,
        mir::geometry::Size size,
        mir::geometry::Stride stride,
        wl_buffer* buffer);
    ~WaylandShmBuffer();

    auto data() const -> void* { return data_; }
    auto use() -> wl_buffer*;

private:
    friend WaylandShm;

    auto is_in_use() const -> bool { return self_ptr != nullptr; }

    static wl_buffer_listener const buffer_listener;

    static void handle_release(void *data, wl_buffer*);

    void* const data_;
    size_t const data_size;
    mir::geometry::Size const size;
    mir::geometry::Stride const stride;
    wl_buffer* const buffer;
    std::shared_ptr<WaylandShmBuffer> self_ptr; ///< Is set on use and cleared on release
};

class WaylandShm
{
public:
    /// Does not take ownership of the wl_shm
    WaylandShm(wl_shm* shm);

    /// The returned buffer is automatically released as long as it is sent to the compositor
    auto get_buffer(mir::geometry::Size size, mir::geometry::Stride stride) -> std::shared_ptr<WaylandShmBuffer>;

private:
    wl_shm* const shm;
    std::shared_ptr<WaylandShmBuffer> current_buffer;
};

#endif // MIRAL_WAYLAND_SHM_H
