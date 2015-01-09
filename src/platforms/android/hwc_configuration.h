/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_ANDROID_HWC_CONFIGURATION_H_
#define MIR_GRAPHICS_ANDROID_HWC_CONFIGURATION_H_

#include "mir/graphics/display_configuration.h"
#include "mir/geometry/size.h"
#include "display_name.h"
#include <memory>

namespace mir
{
namespace graphics
{
namespace android
{
struct DisplayAttribs
{
    geometry::Size pixel_size;
    geometry::Size mm_size;
    double vrefresh_hz;
    bool connected;
};

//interface adapting for the blanking interface differences between fb, HWC 1.0-1.3, and HWC 1.4+
class HwcConfiguration
{
public:
    virtual ~HwcConfiguration() = default;
    virtual void power_mode(DisplayName, MirPowerMode) = 0;
    virtual DisplayAttribs active_attribs_for(DisplayName) = 0; 

protected:
    HwcConfiguration() = default;
    HwcConfiguration(HwcConfiguration const&) = delete;
    HwcConfiguration& operator=(HwcConfiguration const&) = delete;
};

class HwcWrapper;
class HwcBlankingControl : public HwcConfiguration
{
public:
    HwcBlankingControl(std::shared_ptr<HwcWrapper> const&);
    void power_mode(DisplayName, MirPowerMode) override;
    DisplayAttribs active_attribs_for(DisplayName) override;
private:
    std::shared_ptr<HwcWrapper> const hwc_device;
    bool off;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_HWC_CONFIGURATION_H_ */
