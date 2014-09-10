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

#include "device_quirks.h"
#include "output_builder.h"
#include "display_resource_factory.h"
#include "display_buffer.h"
#include "display_device.h"
#include "framebuffers.h"
#include "real_hwc_wrapper.h"

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/egl_resources.h"

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

mga::OutputBuilder::OutputBuilder(
    std::shared_ptr<mga::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<mga::DisplayResourceFactory> const& res_factory,
    std::shared_ptr<mg::DisplayReport> const& display_report,
    mga::OverlayOptimization overlay_optimization,
    std::shared_ptr<HwcLogger> const& logger)
    : buffer_allocator(buffer_allocator),
      res_factory(res_factory),
      display_report(display_report),
      force_backup_display(false),
      overlay_optimization(overlay_optimization)
{
    try
    {
        hwc_native = res_factory->create_hwc_native_device();
        hwc_wrapper = std::make_shared<mga::RealHwcWrapper>(hwc_native, logger);
    } catch (...)
    {
        force_backup_display = true;
    }

    if (force_backup_display || hwc_native->common.version == HWC_DEVICE_API_VERSION_1_0)
    {
        fb_native = res_factory->create_fb_native_device();
        framebuffers = std::make_shared<mga::Framebuffers>(buffer_allocator, fb_native);
    }
    else
    {
        mga::PropertiesOps ops;
        mga::DeviceQuirks quirks(ops);
        framebuffers = std::make_shared<mga::Framebuffers>(
            buffer_allocator, hwc_native, quirks.num_framebuffers());
    }
}

MirPixelFormat mga::OutputBuilder::display_format()
{
    return framebuffers->fb_format();
}

std::unique_ptr<mga::ConfigurableDisplayBuffer> mga::OutputBuilder::create_display_buffer(
    GLProgramFactory const& gl_program_factory,
    GLContext const& gl_context)
{
    std::shared_ptr<mga::DisplayDevice> device;
    if (force_backup_display)
    {
        device = res_factory->create_fb_device(fb_native);
        display_report->report_gpu_composition_in_use();
    }
    else
    {
        if (hwc_native->common.version == HWC_DEVICE_API_VERSION_1_0)
        {
            device = res_factory->create_hwc_fb_device(hwc_wrapper, fb_native);
        }
        else //versions 1.1, 1.2
        {
            device = res_factory->create_hwc_device(hwc_wrapper);
        }

        switch (hwc_native->common.version)
        {
            case HWC_DEVICE_API_VERSION_1_2:
                display_report->report_hwc_composition_in_use(1,2);
                break;
            case HWC_DEVICE_API_VERSION_1_1:
                display_report->report_hwc_composition_in_use(1,1);
                break;
            case HWC_DEVICE_API_VERSION_1_0:
                display_report->report_hwc_composition_in_use(1,0);
                break;
            default:
                break;
        } 
    }

    auto native_window = res_factory->create_native_window(framebuffers);
    return std::unique_ptr<mga::DisplayBuffer>(new DisplayBuffer(
        framebuffers,
        device,
        native_window,
        gl_context,
        gl_program_factory,
        overlay_optimization));
}
