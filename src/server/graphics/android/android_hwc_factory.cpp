/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "android_hwc_factory.h"
#include "hwc11_device.h"
#include "hwc10_device.h"
#include "hwc_layerlist.h"
#include "hwc_vsync.h"

namespace mga=mir::graphics::android;

std::shared_ptr<mga::HWCDevice> mga::AndroidHWCFactory::create_hwc_1_1(
    std::shared_ptr<hwc_composer_device_1> const& hwc_device,
    std::shared_ptr<mga::DisplaySupportProvider> const& fb_device) const
{
    auto layer_list = std::make_shared<mga::LayerList>();
    auto syncer = std::make_shared<mga::HWCVsync>();
    return std::make_shared<mga::HWC11Device>(hwc_device, layer_list, fb_device, syncer);
}

std::shared_ptr<mga::HWCDevice> mga::AndroidHWCFactory::create_hwc_1_0(
    std::shared_ptr<hwc_composer_device_1> const& hwc_device,
    std::shared_ptr<mga::DisplaySupportProvider> const& fb_device) const
{
    auto syncer = std::make_shared<mga::HWCVsync>();
    return std::make_shared<mga::HWC10Device>(hwc_device, fb_device, syncer);
}
