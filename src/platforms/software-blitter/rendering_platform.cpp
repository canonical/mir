/*
 * Copyright © Canonical Ltd.
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
 */

#include "rendering_platform.h"
#include "blitter_provider.h"

#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/graphics/shm_buffer.h>
#include <mir/module_deleter.h>

#include <stdexcept>

namespace mg = mir::graphics;
namespace mgc = mg::common;
namespace mgsb = mg::software_blitter;
namespace geom = mir::geometry;

namespace
{
class StubBufferAllocator : public mg::GraphicBufferAllocator
{
public:
    auto supported_pixel_formats() -> std::vector<MirPixelFormat> override
    {
        static std::vector<MirPixelFormat> const pixel_formats{mir_pixel_format_argb_8888, mir_pixel_format_xrgb_8888};

        return pixel_formats;
    }

    auto alloc_software_buffer(geom::Size size, MirPixelFormat format) -> std::shared_ptr<mg::Buffer> override
    {
        if (!mgc::MemoryBackedShmBuffer::supports(format))
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("Trying to create SHM buffer with unsupported pixel format"));
        }

        return std::make_shared<mgc::MemoryBackedShmBuffer>(size, format);
    }

    void bind_display(wl_display* /*display*/, std::shared_ptr<mir::Executor> /*wayland_executor*/) override {}

    void unbind_display(wl_display* /*display*/) override {}

    auto buffer_from_resource(
        wl_resource* /*buffer*/,
        std::function<void()>&& /*on_consumed*/,
        std::function<void()>&& /*on_release*/) -> std::shared_ptr<mg::Buffer> override
    {
        throw std::runtime_error{"Software-blitter buffer_from_resource not yet implemented"};
    }

    auto buffer_from_shm(
        std::shared_ptr<mir::renderer::software::RWMappable> data,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release) -> std::shared_ptr<mg::Buffer> override
    {
        return std::make_shared<mgc::NotifyingMappableBackedShmBuffer>(
            std::move(data), std::move(on_consumed), std::move(on_release));
    }
};
}

mgsb::SoftwareBlitterRenderingPlatform::SoftwareBlitterRenderingPlatform() : RenderingPlatform() {}

mgsb::SoftwareBlitterRenderingPlatform::~SoftwareBlitterRenderingPlatform() = default;

auto mgsb::SoftwareBlitterRenderingPlatform::maybe_create_provider(RenderingProvider::Tag const& type_tag)
    -> std::shared_ptr<RenderingProvider>
{
    if (dynamic_cast<mg::BlitterRenderingProvider::Tag const*>(&type_tag))
    {
        return std::make_shared<mgsb::SoftwareBlitterRenderingProvider>();
    }
    return nullptr;
}

auto mgsb::SoftwareBlitterRenderingPlatform::create_buffer_allocator(graphics::Display const& /*output*/)
    -> mir::UniqueModulePtr<graphics::GraphicBufferAllocator>
{
    return mir::make_module_ptr<StubBufferAllocator>();
}
