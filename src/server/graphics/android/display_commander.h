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

#include <mir_toolkit/common.h>
#include "mir/geometry/size.h"
#include "mir/geometry/pixel_format.h"

#include <EGL/egl.h>
#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;

namespace android
{

class DisplayCommander
{
public:
    virtual ~DisplayCommander() = default;

    virtual void set_next_frontbuffer(std::shared_ptr<graphics::Buffer> const& buffer) = 0;
    virtual void sync_to_display(bool sync) = 0;
    virtual void mode(MirPowerMode mode) = 0;
    virtual void commit_frame(EGLDisplay dpy, EGLSurface sur) = 0;

protected:
    DisplayCommander() = default;
    DisplayCommander& operator=(DisplayCommander const&) = delete;
    DisplayCommander(DisplayCommander const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DISPLAY_SUPPORT_PROVIDER_H_ */
