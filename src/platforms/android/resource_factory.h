/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_GRAPHICS_ANDROID_RESOURCE_FACTORY_H_
#define MIR_GRAPHICS_ANDROID_RESOURCE_FACTORY_H_

#include "display_resource_factory.h"

namespace mir
{
namespace graphics
{
namespace android
{

class ResourceFactory : public DisplayResourceFactory
{
public:
    //native allocations
    std::tuple<std::shared_ptr<HwcWrapper>, HwcVersion> create_hwc_wrapper(
        std::shared_ptr<HwcReport> const&) const override;
    std::shared_ptr<framebuffer_device_t> create_fb_native_device() const override;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DEFAULT_FRAMEBUFFER_FACTORY_H_ */
