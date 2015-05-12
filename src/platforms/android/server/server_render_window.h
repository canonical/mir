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
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_SERVER_RENDER_WINDOW_H_
#define MIR_GRAPHICS_ANDROID_SERVER_RENDER_WINDOW_H_

#include "mir/graphics/android/android_driver_interpreter.h"
#include "mir_toolkit/common.h"

#include <memory>

namespace mir
{
namespace graphics
{
namespace android
{

class FramebufferBundle;
class InterpreterResourceCache;
class ServerRenderWindow : public AndroidDriverInterpreter
{
public:
    ServerRenderWindow(std::shared_ptr<FramebufferBundle> const& fb_bundle,
                       MirPixelFormat format,
                       std::shared_ptr<InterpreterResourceCache> const&);

    graphics::NativeBuffer* driver_requests_buffer() override;
    void driver_returns_buffer(ANativeWindowBuffer*, int fence_fd) override;
    void dispatch_driver_request_format(int format) override;
    void dispatch_driver_request_buffer_count(unsigned int count) override;
    int driver_requests_info(int key) const override;
    void sync_to_display(bool sync) override;

private:
    std::shared_ptr<FramebufferBundle> const fb_bundle;
    std::shared_ptr<InterpreterResourceCache> const resource_cache;
    int format;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_SERVER_RENDER_WINDOW_H_ */
