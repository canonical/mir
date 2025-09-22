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

#ifndef MIRAL_WAYLAND_SHM_H
#define MIRAL_WAYLAND_SHM_H

#include "mir/geometry/size.h"

#include <wayland-client.h>
#include <memory>
#include <vector>

namespace miral::tk
{
class WaylandShmPool;

/// Describes a shared memory buffer.
class WaylandShmBuffer : public std::enable_shared_from_this<WaylandShmBuffer>
{
public:
    ~WaylandShmBuffer();

    /// \returns the raw data
    auto data() const -> void* { return data_; }

    /// \returns the size
    auto size() const -> mir::geometry::Size { return size_; }

    /// \returns the stride
    auto stride() const -> mir::geometry::Stride { return stride_; }

    /// Marks this buffer as in-use and assumes the resulting #wl_buffer is sent to the compositor.
    ///
    /// Keeps this buffer  alive until the compositor releases it. If the buffer is not sent to the compositor
    /// or the compositor never releases it, this buffer is leaked. This method should only be called if
    /// the buffer is not already in use.
    ///
    /// \returns the wayland buffer object
    auto use() -> wl_buffer*;

private:
    class PoolHandle;
    friend WaylandShmPool;

    WaylandShmBuffer(
        std::shared_ptr<PoolHandle> const& pool,
        void* data,
        mir::geometry::Size const& size,
        mir::geometry::Stride const& stride,
        wl_buffer* buffer);

    /// Returns if this buffer is currently being used by the compositor.
    ///
    /// In-use buffers keep themselves alive.
    ///
    /// \returns `true` if in use, otherwise `false`.
    auto is_in_use() const -> bool { return self_ptr != nullptr; }

    static wl_buffer_listener const buffer_listener;
    static void handle_release(void *data, wl_buffer*);

    std::shared_ptr<PoolHandle> pool;
    void* const data_;
    mir::geometry::Size const size_;
    mir::geometry::Stride const stride_;
    wl_buffer* const buffer;
    std::shared_ptr<WaylandShmBuffer> self_ptr; ///< Is set on use and cleared on release
};

/// A buffer pool that provides access to Wayland shared memory buffers.
///
/// An instance of this class does not efficiently provision multiple buffers for multiple window sizes.
/// Please use one instance of this class per window if windows may have distinct sizes.
class WaylandShmPool
{
public:
    /// Construct a wayland shared pool.
    ///
    /// Does not take ownership of the \p wl_shm.
    ///
    /// \param shm the wl_shm interface
    explicit WaylandShmPool(wl_shm* shm);

    /// Get a buffer.
    ///
    /// Always returns a buffer of the correct size that is not in-use.
    ///
    /// This method assumed that all buffers in the pool have the same size and stride.
    ///
    /// \param size size of the buffer
    /// \param stride stride of the buffer
    /// \returns a buffer
    auto get_buffer(mir::geometry::Size const& size, mir::geometry::Stride const& stride) -> std::shared_ptr<WaylandShmBuffer>;

private:
    wl_shm* const shm;
    std::vector<std::shared_ptr<WaylandShmBuffer>> buffers;
};
}

#endif // MIRAL_WAYLAND_SHM_H
