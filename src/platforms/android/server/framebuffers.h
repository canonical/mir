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

#ifndef MIR_GRAPHICS_ANDROID_FB_SIMPLE_SWAPPER_H_
#define MIR_GRAPHICS_ANDROID_FB_SIMPLE_SWAPPER_H_

#include "framebuffer_bundle.h"

#include <hardware/gralloc.h>
#include <hardware/fb.h>
#include <condition_variable>
#include <queue>
#include <vector>
#include <mutex>

namespace mir
{
namespace graphics
{
namespace android
{
class GraphicBufferAllocator;

class Framebuffers : public FramebufferBundle
{
public:
    Framebuffers(
        GraphicBufferAllocator& buffer_allocator,
        geometry::Size size,
        MirPixelFormat format,
        unsigned int num_framebuffers);

    geometry::Size fb_size() override;
    std::shared_ptr<Buffer> buffer_for_render() override;
    std::shared_ptr<Buffer> last_rendered_buffer() override;

private:
    geometry::Size size;

    std::mutex queue_lock;
    std::shared_ptr<Buffer> buffer_being_rendered;
    std::condition_variable cv;
    std::queue<std::shared_ptr<graphics::Buffer>> queue;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_FB_SIMPLE_SWAPPER_H_ */
