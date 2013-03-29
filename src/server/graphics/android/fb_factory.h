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

#ifndef MIR_GRAPHICS_ANDROID_FB_FACTORY_H_
#define MIR_GRAPHICS_ANDROID_FB_FACTORY_H_

#include <memory>

namespace mir
{
namespace graphics
{
class Display;

namespace android
{

class FBFactory
{
public:
    virtual ~FBFactory() {}

    //creates a display that will render primarily via the gpu and OpenGLES 2.0, but will use the hwc
    //module (version 1.1) for additional functionality, such as vsync timings, and hotplug detection
    virtual std::shared_ptr<Display> create_hwc1_1_gpu_display() const = 0;

    //creates a display that will render primarily via the gpu and OpenGLES 2.0. Primarily a fall-back mode,
    //this display is similar to what Android does when /system/lib/hw/hwcomposer.*.so modules are not present
    virtual std::shared_ptr<Display> create_gpu_display() const = 0;

protected:
    FBFactory() = default;
    FBFactory& operator=(FBFactory const&) = delete;
    FBFactory(FBFactory const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_FB_FACTORY_H_ */
