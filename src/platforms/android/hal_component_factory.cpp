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

#include "fb_device.h"
#include "device_quirks.h"
#include "hal_component_factory.h"
#include "display_resource_factory.h"
#include "display_buffer.h"
#include "display_device.h"
#include "framebuffers.h"
#include "real_hwc_wrapper.h"
#include "hwc_report.h"
#include "hwc_configuration.h"
#include "hwc_layers.h"
#include "hwc_configuration.h"
#include "hwc_vsync.h"
#include "hwc_device.h"
#include "hwc_fb_device.h"

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/egl_resources.h"

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

mga::HalComponentFactory::HalComponentFactory(
    std::shared_ptr<mga::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<mga::DisplayResourceFactory> const& res_factory,
    mga::OverlayOptimization overlay_optimization,
    std::shared_ptr<HwcReport> const& hwc_report)
    : buffer_allocator(buffer_allocator),
      res_factory(res_factory),
      hwc_report(hwc_report),
      force_backup_display(false),
      overlay_optimization(overlay_optimization)
{
    try
    {
        hwc_native = res_factory->create_hwc_native_device();
        hwc_wrapper = std::make_shared<mga::RealHwcWrapper>(hwc_native, hwc_report);
    } catch (...)
    {
        force_backup_display = true;
    }

    if (force_backup_display || hwc_native->common.version == HWC_DEVICE_API_VERSION_1_0)
        fb_native = res_factory->create_fb_native_device();
}

std::unique_ptr<mga::FramebufferBundle> mga::HalComponentFactory::create_framebuffers(mga::DisplayAttribs const& attribs)
{
    return std::unique_ptr<mga::FramebufferBundle>(new mga::Framebuffers(
        *buffer_allocator,
        attribs.pixel_size,
        attribs.display_format,
        attribs.vrefresh_hz, attribs.num_framebuffers));
}

std::unique_ptr<mga::DisplayDevice> mga::HalComponentFactory::create_display_device()
{
    if (force_backup_display)
    {
        hwc_report->report_legacy_fb_module();
        return std::unique_ptr<mga::DisplayDevice>{new mga::FBDevice(fb_native)};
    }
    else
    {
        hwc_report->report_hwc_version(hwc_native->common.version);
        switch (hwc_native->common.version)
        {
            case HWC_DEVICE_API_VERSION_1_0:
                return std::unique_ptr<mga::DisplayDevice>{
                    new mga::HwcFbDevice(hwc_wrapper, fb_native, std::make_shared<mga::HWCVsync>())};
            case HWC_DEVICE_API_VERSION_1_1:
            case HWC_DEVICE_API_VERSION_1_2:
               return std::unique_ptr<mga::DisplayDevice>(
                    new mga::HwcDevice(
                        hwc_wrapper,
                        std::make_shared<mga::HWCVsync>(),
                        std::make_shared<mga::IntegerSourceCrop>()));
            default:
            case HWC_DEVICE_API_VERSION_1_3:
               return std::unique_ptr<mga::DisplayDevice>(
                    new mga::HwcDevice(
                        hwc_wrapper,
                        std::make_shared<mga::HWCVsync>(),
                        std::make_shared<mga::FloatSourceCrop>()));
        }
    }
}

std::unique_ptr<mga::HwcConfiguration> mga::HalComponentFactory::create_hwc_configuration()
{
    if (force_backup_display || hwc_native->common.version == HWC_DEVICE_API_VERSION_1_0)
        return std::unique_ptr<mga::HwcConfiguration>(new mga::FbControl(fb_native));
    else
        return std::unique_ptr<mga::HwcConfiguration>(new mga::HwcBlankingControl(hwc_wrapper));
}
