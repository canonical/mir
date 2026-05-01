/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_STUB_BUFFER_ALLOCATOR_H_
#define MIR_TEST_DOUBLES_STUB_BUFFER_ALLOCATOR_H_

#include <mir/graphics/graphic_buffer_allocator.h>
#include <wayland-server.h>

namespace mir
{
namespace test
{
namespace doubles
{

class StubBufferAllocator : public graphics::GraphicBufferAllocator
{
public:
    auto alloc_software_buffer(geometry::Size sz, MirPixelFormat pf) -> std::shared_ptr<graphics::Buffer> override;

    auto supported_pixel_formats() -> std::vector<MirPixelFormat> override;

    auto buffer_from_shm(
        std::shared_ptr<renderer::software::RWMappable> data,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release) -> std::shared_ptr<graphics::Buffer> override;

    auto buffer_from_dmabuf(
        graphics::DMABufBuffer const& dmabuf,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release) -> std::shared_ptr<graphics::Buffer> override;
};

}
}
}

#endif // MIR_TEST_DOUBLES_STUB_BUFFER_ALLOCATOR_H_
