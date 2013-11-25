/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_FB_SIMPLE_SWAPPER_H_
#define MIR_GRAPHICS_ANDROID_FB_SIMPLE_SWAPPER_H_

#include "framebuffer_bundle.h"

#include <hardware/gralloc.h>
#include <hardware/fb.h>
#include <hardware/hwcomposer.h>
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
    Framebuffers(std::shared_ptr<GraphicBufferAllocator> const& buffer_allocator,
                 std::shared_ptr<hwc_composer_device_1> const& hwc);
    Framebuffers(std::shared_ptr<GraphicBufferAllocator> const& buffer_allocator,
                 std::shared_ptr<framebuffer_device_t> const& fb);

    geometry::PixelFormat fb_format();
    geometry::Size fb_size();
    std::shared_ptr<Buffer> buffer_for_render();
    std::shared_ptr<Buffer> last_rendered_buffer();
    void wait_for_consumed_buffer(bool);

private:
    geometry::PixelFormat const format;
    geometry::Size const size;

    std::mutex queue_lock;
    std::shared_ptr<Buffer> buffer_being_rendered;
    std::condition_variable cv;
    std::queue<std::shared_ptr<graphics::Buffer>> queue;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_FB_SIMPLE_SWAPPER_H_ */
