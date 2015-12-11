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
#include "hwc_device.h"
#include "hwc_fb_device.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

mga::HalComponentFactory::HalComponentFactory(
    std::shared_ptr<mga::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<mga::DisplayResourceFactory> const& res_factory,
    std::shared_ptr<HwcReport> const& hwc_report,
    std::shared_ptr<mga::DeviceQuirks> const& quirks)
    : buffer_allocator(buffer_allocator),
      res_factory(res_factory),
      hwc_report(hwc_report),
      force_backup_display(false),
      num_framebuffers{quirks->num_framebuffers()}
{
    try
    {
        std::tie(hwc_wrapper, hwc_version) = res_factory->create_hwc_wrapper(hwc_report);
        hwc_report->set_version(hwc_version);
    } catch (...)
    {
        force_backup_display = true;
    }

    if (force_backup_display || hwc_version == mga::HwcVersion::hwc10)
    {
        fb_native = res_factory->create_fb_native_device();
        //guarantee always 2 fb's allocated
        num_framebuffers = std::max(2u, static_cast<unsigned int>(fb_native->numFramebuffers));
    }
}

std::unique_ptr<mga::FramebufferBundle> mga::HalComponentFactory::create_framebuffers(mg::DisplayConfigurationOutput const& config)
{
    return std::unique_ptr<mga::FramebufferBundle>(new mga::Framebuffers(
        *buffer_allocator,
        config.modes[config.current_mode_index].size,
        config.current_format,
        num_framebuffers));
}

std::unique_ptr<mga::LayerList> mga::HalComponentFactory::create_layer_list()
{
    geom::Displacement offset{0,0};
    if (force_backup_display)
        return std::unique_ptr<mga::LayerList>(
            new mga::LayerList(std::make_shared<mga::Hwc10Adapter>(), {}, offset));
    switch (hwc_version)
    {
        case mga::HwcVersion::hwc10:
            return std::unique_ptr<mga::LayerList>(
                new mga::LayerList(std::make_shared<mga::Hwc10Adapter>(), {}, offset));
        case mga::HwcVersion::hwc11:
        case mga::HwcVersion::hwc12:
            return std::unique_ptr<mga::LayerList>(
                new mga::LayerList(std::make_shared<mga::IntegerSourceCrop>(), {}, offset));
        case mga::HwcVersion::hwc13:
        case mga::HwcVersion::hwc14:
            return std::unique_ptr<mga::LayerList>(
                new mga::LayerList(std::make_shared<mga::FloatSourceCrop>(), {}, offset));
        case mga::HwcVersion::unknown:
        default:
            BOOST_THROW_EXCEPTION(std::runtime_error("unknown or unsupported hwc version"));
    }
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
        hwc_report->report_hwc_version(hwc_version);
        switch (hwc_version)
        {
            case mga::HwcVersion::hwc10:
                return std::unique_ptr<mga::DisplayDevice>{
                    new mga::HwcFbDevice(hwc_wrapper, fb_native)};

            case mga::HwcVersion::hwc11:
            case mga::HwcVersion::hwc12:
            case mga::HwcVersion::hwc13:
            case mga::HwcVersion::hwc14:
               return std::unique_ptr<mga::DisplayDevice>(
                    new mga::HwcDevice(hwc_wrapper));

            case mga::HwcVersion::unknown:
            default:
                BOOST_THROW_EXCEPTION(std::runtime_error("unknown or unsupported hwc version"));
        }
    }
}

std::unique_ptr<mga::HwcConfiguration> mga::HalComponentFactory::create_hwc_configuration()
{
    if (force_backup_display || hwc_version == mga::HwcVersion::hwc10)
        return std::unique_ptr<mga::HwcConfiguration>(new mga::FbControl(fb_native));
    else if (hwc_version < mga::HwcVersion::hwc14)
        return std::unique_ptr<mga::HwcConfiguration>(new mga::HwcBlankingControl(hwc_wrapper));
    else
        return std::unique_ptr<mga::HwcConfiguration>(new mga::HwcPowerModeControl(hwc_wrapper));
}
