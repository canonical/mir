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

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_FB_FACTORY_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_FB_FACTORY_H_

#include "fb_factory.h"

namespace mir
{
namespace graphics
{
namespace android
{

class HWCFactory;
class HWCDevice;
class DisplayFactory;
class AndroidFBFactory : public FBFactory
{
public:
    AndroidFBFactory(std::shared_ptr<DisplayFactory> const& fb_factory, std::shared_ptr<HWCFactory> const& hwc_factory);

    std::shared_ptr<Display> create_fb() const;

private:
    std::shared_ptr<HWCFactory> hwc_factory;
    std::shared_ptr<HWCDevice> hwc_device;
    bool is_hwc_capable;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_ANDROID_FB_FACTORY_H_ */
