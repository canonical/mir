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

#include "hwc_formatted_logger.h"
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

struct LayerNumber{ unsigned int const num; };
std::ostream& operator<<(std::ostream& str, LayerNumber l)
{
    return str << std::setw(layer_num_column_size) << l.num % 100 << std::setw(0);
}

struct HwcRotation{ unsigned int const key; };
std::ostream& operator<<(std::ostream& str, HwcRotation rotation_key)
{
    str << std::setw(rotation_column_size) << std::left;
    switch(rotation_key.key)
    {
        case 0:
            str << std::string{"NONE"};
            break;
        case HWC_TRANSFORM_ROT_90:
            str << std::string{"ROT_90"}; 
            break;
        case HWC_TRANSFORM_ROT_180:
            str << std::string{"ROT_180"}; 
            break;
        case HWC_TRANSFORM_ROT_270:
            str << std::string{"ROT_270"};
            break;
        default:
            str << std::string{"UNKNOWN"};
            break;
    }
    return str << std::setw(0) << std::right;
}

struct HwcBlending{ int const key; };
std::ostream& operator<<(std::ostream& str, HwcBlending blending_key)
{
    str << std::setw(blending_column_size) << std::left;
    switch(blending_key.key)
    {
        case HWC_BLENDING_NONE:
            str << std::string{"NONE"};
            break;
        case HWC_BLENDING_PREMULT:
            str << std::string{"PREMULT"};
            break;
        case HWC_BLENDING_COVERAGE:
            str << std::string{"COVERAGE"};
            break;
        default:
            str << std::string{"UNKNOWN"};
            break;
    }
    return str << std::setw(0) << std::right;
}

struct HwcType{ int const type; unsigned int const flags; };
std::ostream& operator<<(std::ostream& str, HwcType type)
{
    str << std::setw(type_column_size) << std::left;
    switch(type.type)
    {
        case HWC_OVERLAY:
            str << std::string{"OVERLAY"};
            break;
        case HWC_FRAMEBUFFER:
            if (type.flags == HWC_SKIP_LAYER)
                str << std::string{"FORCE_GL"};
            else
                str << std::string{"GL_RENDER"};
            break;
        case HWC_FRAMEBUFFER_TARGET:
            str << std::string{"FB_TARGET"};
            break;
        default:
            str << std::string{"UNKNOWN"};
            break;
    }
    return str << std::setw(0) << std::right;
}

struct HwcRect { hwc_rect_t const& rect; };
std::ostream& operator<<(std::ostream& str, HwcRect r)
{
    auto rect = r.rect;
    return str << "{"
               << std::setw(rect_entry_column_size) << rect.left << ","
               << std::setw(rect_entry_column_size) << rect.top << ","
               << std::setw(rect_entry_column_size) << rect.right << ","
               << std::setw(rect_entry_column_size) << rect.bottom << "}"
               << std::setw(0);
}
}

void mga::HwcFormattedLogger::log_list_submitted_to_prepare(hwc_display_contents_1_t const& list) const
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

void mga::HwcFormattedLogger::log_prepare_done(hwc_display_contents_1_t const& list) const
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

void mga::HwcFormattedLogger::log_set_list(hwc_display_contents_1_t const& list) const
{
    std::cout << "set list():" << std::endl
              << " # | handle" << std::endl;

    for(auto i = 0u; i < list.numHwLayers; i++)
        std::cout << LayerNumber{i}
                  << separator
                  << list.hwLayers[i].handle
                  << std::endl;
}
