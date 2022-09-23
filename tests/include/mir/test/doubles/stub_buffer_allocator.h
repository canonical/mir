/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir/graphics/graphic_buffer_allocator.h"
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

    void bind_display(wl_display*, std::shared_ptr<mir::Executor>) override;

    void unbind_display(wl_display*) override;

    auto buffer_from_resource(wl_resource*, std::function<void()>&&, std::function<void()>&&)
        -> std::shared_ptr<graphics::Buffer> override;

    auto buffer_from_shm(
        wl_resource* resource,
        std::shared_ptr<mir::Executor> executor,
        std::function<void()>&& on_consumed) -> std::shared_ptr<graphics::Buffer> override;

private:
    std::shared_ptr<void> sigbus_handler;
};

}
}
}

#endif // MIR_TEST_DOUBLES_STUB_BUFFER_ALLOCATOR_H_
