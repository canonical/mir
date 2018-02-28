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
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_FRONTEND_WLSHMBUFFER_H_
#define MIR_FRONTEND_WLSHMBUFFER_H_

#include <mir/graphics/buffer_basic.h>
#include <mir/renderer/gl/texture_source.h>
#include <mir/renderer/sw/pixel_source.h>

#include <wayland-server-core.h>

#include <functional>
#include <mutex>
#include <string>

namespace mir
{
namespace frontend
{

class WlShmBuffer :
    public graphics::BufferBasic,
    public graphics::NativeBufferBase,
    public renderer::gl::TextureSource,
    public renderer::software::PixelSource
{
public:
    ~WlShmBuffer();

    static std::shared_ptr <graphics::Buffer> mir_buffer_from_wl_buffer(
        wl_resource *buffer,
        std::function<void()> &&on_consumed);

    std::shared_ptr <graphics::NativeBuffer> native_buffer_handle() const override;

    geometry::Size size() const override;

    MirPixelFormat pixel_format() const override;

    graphics::NativeBufferBase *native_buffer_base() override;

    void gl_bind_to_texture() override;

    void bind() override;

    void secure_for_render() override;

    void write(unsigned char const *pixels, size_t size) override;

    void read(std::function<void(unsigned char const *)> const &do_with_pixels) override;

    geometry::Stride stride() const override;

private:
    WlShmBuffer(
        wl_resource *buffer,
        std::function<void()> &&on_consumed);

    static void on_buffer_destroyed(wl_listener *listener, void *);

    struct DestructionShim
    {
        std::shared_ptr <std::mutex> const mutex = std::make_shared<std::mutex>();
        std::weak_ptr <WlShmBuffer> associated_buffer;
        wl_listener destruction_listener;
    };

    std::shared_ptr <std::mutex> buffer_mutex;

    wl_shm_buffer *buffer;
    wl_resource *const resource;

    geometry::Size const size_;
    geometry::Stride const stride_;
    MirPixelFormat const format_;

    std::unique_ptr<uint8_t[]> const data;

    bool consumed;
    std::function<void()> on_consumed;
};
}
}

#endif