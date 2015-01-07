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

#include "hwc_loggers.h"
#include <iostream>
#include <iomanip>

namespace mga=mir::graphics::android;

namespace
{
std::string const separator{" | "};
int const layer_num_column_size{2};
int const blending_column_size{8};
int const rotation_column_size{9};
int const rect_entry_column_size{4};
int const type_column_size{9};

class StreamFormatter
{
public:
    StreamFormatter(std::ostream& str, unsigned int const width, std::ios_base::fmtflags flags)
    : stream(str),
      old_width(stream.width(width)),
      old_flags(stream.setf(flags,std::ios::adjustfield))
    {
    }

   ~StreamFormatter()
    {
        stream.setf(old_flags, std::ios::adjustfield);
        stream.width(old_width);
    }
private:
    std::ostream& stream;
    unsigned int const old_width;
    std::ios_base::fmtflags const old_flags;
};

struct LayerNumber{ unsigned int const num; };
std::ostream& operator<<(std::ostream& str, LayerNumber l)
{
    StreamFormatter stream_format(str, layer_num_column_size, std::ios_base::right);
    return str << l.num % 100;
}

struct HwcRotation{ unsigned int const key; };
std::ostream& operator<<(std::ostream& str, HwcRotation rotation_key)
{
    StreamFormatter stream_format(str, rotation_column_size, std::ios_base::left);
    switch(rotation_key.key)
    {
        case 0:
            return str << std::string{"NONE"};
        case HWC_TRANSFORM_ROT_90:
            return str << std::string{"ROT_90"}; 
        case HWC_TRANSFORM_ROT_180:
            return str << std::string{"ROT_180"}; 
        case HWC_TRANSFORM_ROT_270:
            return str << std::string{"ROT_270"};
        default:
            return str << std::string{"UNKNOWN"};
    }
}

struct HwcBlending{ int const key; };
std::ostream& operator<<(std::ostream& str, HwcBlending blending_key)
{
    StreamFormatter stream_format(str, blending_column_size, std::ios_base::left);
    switch(blending_key.key)
    {
        case HWC_BLENDING_NONE:
            return str << std::string{"NONE"};
        case HWC_BLENDING_PREMULT:
            return str << std::string{"PREMULT"};
        case HWC_BLENDING_COVERAGE:
            return str << std::string{"COVERAGE"};
        default:
            return str << std::string{"UNKNOWN"};
    }
}

struct HwcType{ int const type; unsigned int const flags; };
std::ostream& operator<<(std::ostream& str, HwcType type)
{
    StreamFormatter stream_format(str, type_column_size, std::ios_base::left);
    switch(type.type)
    {
        case HWC_OVERLAY:
            return str << std::string{"OVERLAY"};
        case HWC_FRAMEBUFFER:
            if (type.flags == HWC_SKIP_LAYER)
                return str << std::string{"FORCE_GL"};
            else
                return str << std::string{"GL_RENDER"};
        case HWC_FRAMEBUFFER_TARGET:
            return str << std::string{"FB_TARGET"};
        default:
            return str << std::string{"UNKNOWN"};
    }
}

struct HwcRectMember { int member; };
std::ostream& operator<<(std::ostream& str, HwcRectMember rect) 
{
    StreamFormatter stream_format(str, rect_entry_column_size, std::ios_base::right);
    return str << rect.member; 
}

struct HwcRect { hwc_rect_t const& rect; };
std::ostream& operator<<(std::ostream& str, HwcRect r)
{
    return str << "{"
               << HwcRectMember{r.rect.left} << ","
               << HwcRectMember{r.rect.top} << ","
               << HwcRectMember{r.rect.right} << ","
               << HwcRectMember{r.rect.bottom} << "}";
}

std::ostream& operator<<(std::ostream& str, mga::OverlayOptimization opt)
{
    if (opt == mga::OverlayOptimization::enabled)
        return str << "ON";
    else
        return str << "OFF";
}
}

void mga::HwcFormattedLogger::report_list_submitted_to_prepare(hwc_display_contents_1_t const& list) const
{
    std::cout << "before prepare():" << std::endl
              << " # | pos {l,t,r,b}         | crop {l,t,r,b}        | transform | blending | "
              << std::endl;
    for(auto i = 0u; i < list.numHwLayers; i++)
        std::cout << LayerNumber{i}
                  << separator
                  << HwcRect{list.hwLayers[i].displayFrame}
                  << separator
                  << HwcRect{list.hwLayers[i].sourceCrop}
                  << separator
                  << HwcRotation{list.hwLayers[i].transform}
                  << separator
                  << HwcBlending{list.hwLayers[i].blending}
                  << separator
                  << std::endl;
}

void mga::HwcFormattedLogger::report_prepare_done(hwc_display_contents_1_t const& list) const
{
    std::cout << "after prepare():" << std::endl
              << " # | Type      | " << std::endl;
    for(auto i = 0u; i < list.numHwLayers; i++)
        std::cout << LayerNumber{i}
                  << separator
                  << HwcType{list.hwLayers[i].compositionType,list.hwLayers[i].flags}
                  << separator
                  << std::endl;
}

void mga::HwcFormattedLogger::report_set_list(hwc_display_contents_1_t const& list) const
{
    std::cout << "set list():" << std::endl
              << " # | handle" << std::endl;

    for(auto i = 0u; i < list.numHwLayers; i++)
        std::cout << LayerNumber{i}
                  << separator
                  << list.hwLayers[i].handle
                  << std::endl;
}

void mga::HwcFormattedLogger::report_overlay_optimization(OverlayOptimization overlay_optimization) const
{
    std::cout << "HWC overlay optimizations are " << overlay_optimization << std::endl;
}

void mga::HwcFormattedLogger::report_display_on() const
{
    std::cout << "HWC: display on" << std::endl;
}

void mga::HwcFormattedLogger::report_display_off() const
{
    std::cout << "HWC: display off" << std::endl;
}

void mga::HwcFormattedLogger::report_vsync_on() const
{
    std::cout << "HWC: vsync signal on" << std::endl;
}

void mga::HwcFormattedLogger::report_vsync_off() const
{
    std::cout << "HWC: vsync signal off" << std::endl;
}

void mga::HwcFormattedLogger::report_hwc_version(uint32_t version) const
{
    std::cout << "HWC version ";
    switch (version)
    {
        case HWC_DEVICE_API_VERSION_1_0:
            std::cout << "1.0";
            break;
        case HWC_DEVICE_API_VERSION_1_1:
            std::cout << "1.1";
            break;
        case HWC_DEVICE_API_VERSION_1_2:
            std::cout << "1.2";
            break;
        case HWC_DEVICE_API_VERSION_1_3:
            std::cout << "1.3";
            break;
        default:
            std::cout << "unknown (0x" << std::hex << version << std::dec << ")";
            break;
    }
    std::cout << std::endl;
}

void mga::HwcFormattedLogger::report_legacy_fb_module() const
{
    std::cout << "Legacy FB module" << std::endl;
}

void mga::NullHwcReport::report_list_submitted_to_prepare(hwc_display_contents_1_t const&) const {}
void mga::NullHwcReport::report_prepare_done(hwc_display_contents_1_t const&) const {}
void mga::NullHwcReport::report_set_list(hwc_display_contents_1_t const&) const {}
void mga::NullHwcReport::report_overlay_optimization(OverlayOptimization) const {}
void mga::NullHwcReport::report_display_on() const {}
void mga::NullHwcReport::report_display_off() const {}
void mga::NullHwcReport::report_vsync_on() const {}
void mga::NullHwcReport::report_vsync_off() const {}
void mga::NullHwcReport::report_hwc_version(uint32_t) const {}
void mga::NullHwcReport::report_legacy_fb_module() const {}
