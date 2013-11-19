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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_DISPLAY_BUFFER_H_
#define MIR_GRAPHICS_ANDROID_DISPLAY_BUFFER_H_

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/egl_resources.h"
#include <system/window.h>

namespace mir
{
namespace graphics
{
namespace android
{

class DisplayDevice;
class DisplayBuffer : public graphics::DisplayBuffer
{
public:
    DisplayBuffer(std::shared_ptr<DisplayDevice> const& display_device,
                  std::shared_ptr<ANativeWindow> const& native_window,
                  EGLDisplay connection, EGLConfig config, EGLContext shared_context);

    geometry::Rectangle view_area() const;
    void make_current();
    void release_current();
    void post_update();
    bool can_bypass() const override;
    
private:
    std::shared_ptr<DisplayDevice> const display_device;
    std::shared_ptr<ANativeWindow> const native_window;

    EGLDisplay const egl_display;
    EGLContextStore const egl_context;
    EGLSurfaceStore const egl_surface;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DISPLAY_BUFFER_H_ */
