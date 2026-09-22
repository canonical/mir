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

#include "blitter_provider.h"

#include <mir/geometry/rectangle.h>

namespace mg = mir::graphics;
namespace mgsb = mg::software_blitter;

class mg::BlitterRenderingProvider::Surface
{
public:
    Surface() = default;
    virtual ~Surface() = default;
};

class mg::BlitterRenderingProvider::Task
{
public:
    Task() = default;
    virtual ~Task() = default;
};

mgsb::SoftwareBlitterRenderingProvider::SoftwareBlitterRenderingProvider() : mg::BlitterRenderingProvider() {}

auto mgsb::SoftwareBlitterRenderingProvider::create_task(std::unique_ptr<Surface> /*surf*/) -> std::unique_ptr<Task>
{
    return std::make_unique<Task>();
}

auto mgsb::SoftwareBlitterRenderingProvider::wait_complete(std::unique_ptr<Task> task) -> std::unique_ptr<Surface>
{
    (void)task;
    return nullptr;
}

auto mgsb::SoftwareBlitterRenderingProvider::blit(
    Task& /*task*/,
    Buffer const& /*source*/,
    geometry::Rectangle const& /*source_rect*/,
    geometry::Rectangle const& /*target_rect*/,
    MirOrientation /*rotation*/,
    MirMirrorMode /*mirror_mode*/) -> bool
{
    return false;
}

auto mgsb::SoftwareBlitterRenderingProvider::fill(
    Task& /*task*/,
    geometry::Rectangle const& /*target_rect*/,
    uint8_t /*r*/,
    uint8_t /*g*/,
    uint8_t /*b*/,
    uint8_t /*a*/) -> bool
{
    return false;
}

auto mgsb::SoftwareBlitterRenderingProvider::surface_for_fb(CPUAddressableDisplayAllocator::MappableFB const& /*fb*/)
    -> std::unique_ptr<Surface>
{
    return std::make_unique<Surface>();
}

auto mgsb::SoftwareBlitterRenderingProvider::map_buffer(Buffer const& /*buffer*/)
    -> std::unique_ptr<renderer::software::Mapping<std::byte const>>
{
    return nullptr;
}

auto mgsb::SoftwareBlitterRenderingProvider::suitability_for_display(DisplaySink& /*sink*/) -> probe::Result
{
    return probe::supported;
}

auto mgsb::SoftwareBlitterRenderingProvider::suitability_for_allocator(
    std::shared_ptr<GraphicBufferAllocator> const& /*target*/) -> probe::Result
{
    return probe::supported;
}

auto mgsb::SoftwareBlitterRenderingProvider::make_framebuffer_provider(DisplaySink& /*sink*/)
    -> std::unique_ptr<FramebufferProvider>
{
    return nullptr;
}
