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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_FB_DEVICE_H_
#define MIR_GRAPHICS_ANDROID_FB_DEVICE_H_

#include "display_device.h"
#include <hardware/gralloc.h>
#include <hardware/fb.h>

namespace mir
{
namespace graphics
{
namespace android
{

class FBDevice : public DisplayDevice
{
public:
    FBDevice(std::shared_ptr<framebuffer_device_t> const& fbdev);

    void mode(MirPowerMode mode);
    void prepare_gl();
    void prepare_gl_and_overlays(std::list<std::shared_ptr<Renderable>> const& list); 
    void gpu_render(EGLDisplay dpy, EGLSurface sur);
    void post(Buffer const& buffer);

private:
    std::shared_ptr<framebuffer_device_t> const fb_device;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_FB_DEVICE_H_ */
