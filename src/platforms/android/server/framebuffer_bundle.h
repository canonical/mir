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

#ifndef MIR_GRAPHICS_ANDROID_FRAMEBUFFER_BUNDLE_H_
#define MIR_GRAPHICS_ANDROID_FRAMEBUFFER_BUNDLE_H_

#include "mir_toolkit/common.h"
#include "mir/geometry/size.h"
#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;

namespace android
{

class FramebufferBundle{
public:
    virtual ~FramebufferBundle() = default;

    virtual geometry::Size fb_size() = 0;
    virtual std::shared_ptr<Buffer> buffer_for_render() = 0;
    virtual std::shared_ptr<Buffer> last_rendered_buffer() = 0;

protected:
    FramebufferBundle() = default;
    FramebufferBundle(FramebufferBundle const&) = delete;
    FramebufferBundle& operator=(FramebufferBundle const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_FRAMEBUFFER_BUNDLE_H_ */
