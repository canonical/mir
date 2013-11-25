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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "output_builder.h"
#include "display_resource_factory.h"
#include "display_buffer.h"
#include "display_device.h"
#include "framebuffers.h"

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
    bool should_use_fb_fallback)
    : buffer_allocator(buffer_allocator),
      res_factory(res_factory),
      display_report(display_report),
      force_backup_display(should_use_fb_fallback)
{
    if (!force_backup_display)
    {
        try
        {
            hwc_native = res_factory->create_hwc_native_device();
        } catch (...)
        {
            force_backup_display = true;
        }

        //HWC 1.2 not supported yet. make an attempt to use backup display
        if (hwc_native && hwc_native->common.version == HWC_DEVICE_API_VERSION_1_2)
        {
            force_backup_display = true;
        }
    }

    if (force_backup_display || hwc_native->common.version == HWC_DEVICE_API_VERSION_1_0)
    {
        fb_native = res_factory->create_fb_native_device();
        framebuffers = std::make_shared<mga::Framebuffers>(buffer_allocator, fb_native);
    }
    else
    {
        framebuffers = std::make_shared<mga::Framebuffers>(buffer_allocator, hwc_native);
    }
}

geom::PixelFormat mga::OutputBuilder::display_format()
{
    return framebuffers->fb_format();
}

std::shared_ptr<mga::DisplayDevice> mga::OutputBuilder::create_display_device()
{
    std::shared_ptr<mga::DisplayDevice> device; 
    if (force_backup_display)
    {
        device = res_factory->create_fb_device(fb_native);
        display_report->report_gpu_composition_in_use();
    }
    else
    {
        if (hwc_native->common.version == HWC_DEVICE_API_VERSION_1_1)
        {
            device = res_factory->create_hwc11_device(hwc_native);
            display_report->report_hwc_composition_in_use(1,1);
        }
        if (hwc_native->common.version == HWC_DEVICE_API_VERSION_1_0)
        {
            device = res_factory->create_hwc10_device(hwc_native, fb_native);
            display_report->report_hwc_composition_in_use(1,0);
        }
    }

    return device;
}

std::unique_ptr<mg::DisplayBuffer> mga::OutputBuilder::create_display_buffer(
    std::shared_ptr<DisplayDevice> const& display_device,
    GLContext const& gl_context)
{
    auto native_window = res_factory->create_native_window(framebuffers);
    return std::unique_ptr<mg::DisplayBuffer>(
        new DisplayBuffer(framebuffers, display_device, native_window, gl_context)); 
}
