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

#ifndef MIR_GRAPHICS_ANDROID_REAL_HWC_WRAPPER_H_
#define MIR_GRAPHICS_ANDROID_REAL_HWC_WRAPPER_H_

#include "hwc_device.h"
#include <memory>
#include <hardware/hwcomposer.h>

namespace mir
{
namespace graphics
{
namespace android
{

class RealHwcWrapper : public HwcWrapper
{
public:
    RealHwcWrapper(std::shared_ptr<hwc_composer_device_1> const& hwc_device);

    void prepare(hwc_display_contents_1_t&) const override;
    void set(hwc_display_contents_1_t&) const override;
private:
    static size_t const num_displays{3}; //primary, external, virtual
    std::shared_ptr<hwc_composer_device_1> const hwc_device;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_REAL_HWC_WRAPPER_H_ */
