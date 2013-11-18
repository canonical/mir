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

#ifndef MIR_GRAPHICS_ANDROID_DISPLAY_RESOURCE_FACTORY_H_
#define MIR_GRAPHICS_ANDROID_DISPLAY_RESOURCE_FACTORY_H_

#include <system/window.h>
#include <hardware/hwcomposer.h>
#include <memory>

namespace mir
{
namespace graphics
{
class Display;
class DisplayReport;

namespace android
{
class DisplayDevice;
class FramebufferBundle;

class DisplayResourceFactory
{
public:
    virtual ~DisplayResourceFactory() = default;

    virtual std::shared_ptr<hwc_composer_device_1> create_hwc_native_device() const = 0;
    virtual std::shared_ptr<framebuffer_device_t> create_fb_native_device() const = 0;

    virtual std::shared_ptr<ANativeWindow> create_native_window(
        std::shared_ptr<FramebufferBundle> const& device) const = 0;

    virtual std::shared_ptr<DisplayDevice> create_fb_device(
        std::shared_ptr<framebuffer_device_t> const& fb_native_device) const = 0;
    virtual std::shared_ptr<DisplayDevice> create_hwc11_device(
        std::shared_ptr<hwc_composer_device_1> const& hwc_native_device) const = 0;
    virtual std::shared_ptr<DisplayDevice> create_hwc10_device(
        std::shared_ptr<hwc_composer_device_1> const& hwc_native_device,
        std::shared_ptr<framebuffer_device_t> const& fb_native_device) const = 0;

protected:
    DisplayResourceFactory() = default;
    DisplayResourceFactory& operator=(DisplayResourceFactory const&) = delete;
    DisplayResourceFactory(DisplayResourceFactory const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_FRAMEBUFFER_FACTORY_H_ */
