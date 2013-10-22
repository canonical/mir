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

#include "mir/graphics/display_report.h"
#include "android_display_factory.h"
#include "display_resource_factory.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;

namespace
{
std::shared_ptr<hwc_composer_device_1> setup_hwc_dev()
{
    const hw_module_t *module;
    int rc = hw_get_module(HWC_HARDWARE_MODULE_ID, &module);
    if ((rc != 0) || (module == nullptr))
    {
        // this is nonfatal, we'll just create the backup display
        return {};
    }

    if ((!module->methods) || !(module->methods->open))
    {
        // TODO: log "display factory cannot create hwc display".
        // this is nonfatal, we'll just create the backup display
        return {};
    }

    hwc_composer_device_1* hwc_device_raw = nullptr;
    rc = module->methods->open(module, HWC_HARDWARE_COMPOSER, reinterpret_cast<hw_device_t**>(&hwc_device_raw));

    if ((rc != 0) || (hwc_device_raw == nullptr))
    {
        // TODO: log "display hwc module unusable".
        // this is nonfatal, we'll just create the backup display
        return {};
    }

    return std::shared_ptr<hwc_composer_device_1>(
        hwc_device_raw,
        [](hwc_composer_device_1* device) { device->common.close((hw_device_t*) device); });
}
}

// TODO this whole class seems to be a dismembered function call - replace with a function
mga::AndroidDisplayFactory::AndroidDisplayFactory(
    std::shared_ptr<DisplayResourceFactory> const& resource_factory,
    std::shared_ptr<DisplayReport> const& display_report)
      : resource_factory(resource_factory),
        fb_dev(resource_factory->create_fb_device()),
        display_report(display_report),
        hwc_dev(setup_hwc_dev())
{
}

std::shared_ptr<mg::Display> mga::AndroidDisplayFactory::create_display() const
{
    std::shared_ptr<mga::DisplaySupportProvider> support_provider;
    //TODO: if hwc display creation fails, we could try the gpu display
    if (hwc_dev && (hwc_dev->common.version == HWC_DEVICE_API_VERSION_1_1))
    {
        support_provider = resource_factory->create_hwc_1_1(hwc_dev, fb_dev);
        display_report->report_hwc_composition_in_use(1,1);
    }
    else if (hwc_dev && (hwc_dev->common.version == HWC_DEVICE_API_VERSION_1_0))
    {
        support_provider = resource_factory->create_hwc_1_0(hwc_dev, fb_dev);
        display_report->report_hwc_composition_in_use(1,0);
    }
    else
    {
        support_provider = fb_dev;
        display_report->report_gpu_composition_in_use();
    }

    return resource_factory->create_display(support_provider, display_report);
}
