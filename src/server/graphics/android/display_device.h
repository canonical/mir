/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_GRAPHICS_ANDROID_DISPLAY_SUPPORT_PROVIDER_H_
#define MIR_GRAPHICS_ANDROID_DISPLAY_SUPPORT_PROVIDER_H_

#include "mir_toolkit/common.h"
#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
class Buffer;

namespace android
{

class DisplayDevice
{
public:
    virtual ~DisplayDevice() = default;

    virtual void mode(MirPowerMode mode) = 0;
    virtual void prepare_composition() = 0;
    virtual void gpu_render(EGLDisplay dpy, EGLSurface sur) = 0; 
    virtual void post(Buffer const& buffer) = 0;

protected:
    DisplayDevice() = default;
    DisplayDevice& operator=(DisplayDevice const&) = delete;
    DisplayDevice(DisplayDevice const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DISPLAY_SUPPORT_PROVIDER_H_ */
