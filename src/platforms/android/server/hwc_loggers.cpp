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

struct DisplayName{ unsigned int const name; };
std::ostream& operator<<(std::ostream& str, DisplayName d)
{
    if (d.name == HWC_DISPLAY_PRIMARY) str <<  "primary ";
    if (d.name == HWC_DISPLAY_EXTERNAL) str << "external";
    return str;
}

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

struct HwcFloatRectMember { float member; };
std::ostream& operator<<(std::ostream& str, HwcFloatRectMember rect)
{
    StreamFormatter stream_format(str, rect_entry_column_size, std::ios_base::right);
    return str << rect.member;
}

struct HwcFloatRect { hwc_frect_t const& rect; };
std::ostream& operator<<(std::ostream& str, HwcFloatRect r)
{
    return str << "{"
               << HwcFloatRectMember{r.rect.left} << ","
               << HwcFloatRectMember{r.rect.top} << ","
               << HwcFloatRectMember{r.rect.right} << ","
               << HwcFloatRectMember{r.rect.bottom} << "}";
}

std::ostream& operator<<(std::ostream& str, mga::OverlayOptimization opt)
{
    if (opt == mga::OverlayOptimization::enabled)
        return str << "ON";
    else
        return str << "OFF";
}

std::ostream& operator<<(std::ostream& str, mga::HwcVersion version) 
{
    switch (version)
    {
        case mga::HwcVersion::hwc10: str << "1.0"; break;
        case mga::HwcVersion::hwc11: str << "1.1"; break;
        case mga::HwcVersion::hwc12: str << "1.2"; break;
        case mga::HwcVersion::hwc13: str << "1.3"; break;
        case mga::HwcVersion::hwc14: str << "1.4"; break;
        default: break;
    }
    return str;
}

std::ostream& operator<<(std::ostream& str, mga::PowerMode power_mode)
{
    switch (power_mode)
    {
        case mga::PowerMode::off: str << "off"; break;
        case mga::PowerMode::doze: str << "doze"; break;
        case mga::PowerMode::normal: str << "on(normal)"; break;
        case mga::PowerMode::doze_suspend: str << "doze(suspend)"; break;
        default: break;
    }
    return str;
}
}

void mga::HwcFormattedLogger::report_list_submitted_to_prepare(
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays) const
{
    std::cout << "before prepare():" << std::endl
              << " # | display  | Type      | pos {l,t,r,b}         | crop {l,t,r,b}        | transform | blending | "
              << std::endl;
    for(auto i = 0u; i < displays.size(); i++)
    {
        if (!displays[i]) continue;
        for(auto j = 0u; j < displays[i]->numHwLayers; j++)
        {
            std::cout << LayerNumber{j}
                      << separator
                      << DisplayName{i}
                      << separator
                      << HwcType{displays[i]->hwLayers[j].compositionType, displays[i]->hwLayers[j].flags}
                      << separator
                      << HwcRect{displays[i]->hwLayers[j].displayFrame}
                      << separator;

            if (hwc_version < HwcVersion::hwc13)
                std::cout << HwcRect{displays[i]->hwLayers[j].sourceCrop};
            else
                std::cout << HwcFloatRect{displays[i]->hwLayers[j].sourceCropf};

            std::cout << separator
                      << HwcRotation{displays[i]->hwLayers[j].transform}
                      << separator
                      << HwcBlending{displays[i]->hwLayers[j].blending}
                      << separator
                      << std::endl;
        }
    }
}

void mga::HwcFormattedLogger::report_prepare_done(
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays) const
{
    std::cout << "after prepare():" << std::endl
              << " # | display  | Type      | " << std::endl;
    for(auto i = 0u; i < displays.size(); i++)
    {
        if (!displays[i]) continue;
        for(auto j = 0u; j < displays[i]->numHwLayers; j++)
            std::cout << LayerNumber{j}
                      << separator
                      << DisplayName{i}
                      << separator
                      << HwcType{displays[i]->hwLayers[j].compositionType, displays[i]->hwLayers[j].flags}
                      << separator
                      << std::endl;
    }
}

void mga::HwcFormattedLogger::report_set_list(
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays) const
{
    std::cout << "set list():" << std::endl
              << " # | display  | Type      | handle | acquireFenceFd" << std::endl;

    for(auto i = 0u; i < displays.size(); i++)
    {
        if (!displays[i]) continue;
        for(auto j = 0u; j < displays[i]->numHwLayers; j++)
            std::cout << LayerNumber{j}
                      << separator
                      << DisplayName{i}
                      << separator
                      << HwcType{displays[i]->hwLayers[j].compositionType, displays[i]->hwLayers[j].flags}
                      << separator
                      << displays[i]->hwLayers[j].handle
                      << separator
                      << displays[i]->hwLayers[j].acquireFenceFd
                      << std::endl;
    }
}

void mga::HwcFormattedLogger::report_set_done(
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const& displays) const
{
    std::cout << "after set():" << std::endl
              << " # | display  | releaseFenceFd" << std::endl;
    for(auto i = 0u; i < displays.size(); i++)
    {
        if (!displays[i]) continue;
        for(auto j = 0u; j < displays[i]->numHwLayers; j++)
            std::cout << LayerNumber{j}
                      << separator
                      << DisplayName{i}
                      << separator
                      << displays[i]->hwLayers[j].releaseFenceFd
                      << std::endl;
    }
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

void mga::HwcFormattedLogger::report_hwc_version(mga::HwcVersion version) const
{
    std::cout << "HWC version " << version << std::endl;
}

void mga::HwcFormattedLogger::report_legacy_fb_module() const
{
    std::cout << "Legacy FB module" << std::endl;
}

void mga::HwcFormattedLogger::report_power_mode(PowerMode mode) const
{
    std::cout << "HWC: power mode: " << mode << std::endl;
}

void mga::NullHwcReport::report_list_submitted_to_prepare(
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&) const {}
void mga::NullHwcReport::report_prepare_done(
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&) const {}
void mga::NullHwcReport::report_set_list(
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&) const {}
void mga::NullHwcReport::report_set_done(
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&) const {}
void mga::NullHwcReport::report_overlay_optimization(OverlayOptimization) const {}
void mga::NullHwcReport::report_display_on() const {}
void mga::NullHwcReport::report_display_off() const {}
void mga::NullHwcReport::report_vsync_on() const {}
void mga::NullHwcReport::report_vsync_off() const {}
void mga::NullHwcReport::report_hwc_version(mga::HwcVersion) const {}
void mga::NullHwcReport::report_legacy_fb_module() const {}
void mga::NullHwcReport::report_power_mode(PowerMode) const {}
