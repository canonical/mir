/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "framebuffers.h"
#include "graphic_buffer_allocator.h"

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::Framebuffers::Framebuffers(
    mga::GraphicBufferAllocator& buffer_allocator,
    geom::Size size,
    MirPixelFormat format,
    unsigned int num_framebuffers) :
    size{size}
{
    for(auto i = 0u; i < num_framebuffers; i++)
        queue.push(buffer_allocator.alloc_buffer_platform(size, format, mga::BufferUsage::use_framebuffer_gles));
}

geom::Size mga::Framebuffers::fb_size()
{
    return size;
}

std::shared_ptr<mg::Buffer> mga::Framebuffers::buffer_for_render()
{
    std::unique_lock<std::mutex> lk(queue_lock);
    while (buffer_being_rendered)
    {
        cv.wait(lk);
    }

    buffer_being_rendered = queue.front();
    queue.pop();
    return std::shared_ptr<mg::Buffer>(buffer_being_rendered.get(),
        [this](mg::Buffer*)
        {
            std::unique_lock<std::mutex> lk(queue_lock);
            queue.push(buffer_being_rendered);
            buffer_being_rendered.reset();
            cv.notify_all();
        });
}

std::shared_ptr<mg::Buffer> mga::Framebuffers::last_rendered_buffer()
{
    std::unique_lock<std::mutex> lk(queue_lock);
    return queue.back();
}
