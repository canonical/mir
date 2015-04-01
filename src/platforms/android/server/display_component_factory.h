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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_DISPLAY_COMPONENT_FACTORY_H_
#define MIR_GRAPHICS_ANDROID_DISPLAY_COMPONENT_FACTORY_H_

#include "display_device.h"
#include "framebuffer_bundle.h"
#include <memory>

namespace mir
{
namespace graphics
{
class DisplayConfigurationOutput;
namespace android
{
class HwcConfiguration;

//TODO: this name needs improvement.
class DisplayComponentFactory
{
public:
    virtual ~DisplayComponentFactory() = default;

    virtual std::unique_ptr<FramebufferBundle> create_framebuffers(DisplayConfigurationOutput const&) = 0;
    virtual std::unique_ptr<DisplayDevice> create_display_device() = 0;
    virtual std::unique_ptr<HwcConfiguration> create_hwc_configuration() = 0;
    virtual std::unique_ptr<LayerList> create_layer_list() = 0;

protected:
    DisplayComponentFactory() = default;
    DisplayComponentFactory(DisplayComponentFactory const&) = delete;
    DisplayComponentFactory& operator=(DisplayComponentFactory const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DISPLAY_COMPONENT_FACTORY_H_ */
