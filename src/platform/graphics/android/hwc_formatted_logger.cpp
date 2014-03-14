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

mga::HwcFormattedLogger::HwcFormattedLogger()
{
}

namespace
{
std::string rotation(int rotation_key)
{
    switch(rotation_key)
    {
        case 0:
            return std::string{"NONE     "};
        case HWC_TRANSFORM_ROT_90:
            return std::string{"ROT_90   "}; 
        case HWC_TRANSFORM_ROT_180:
            return std::string{"ROT_180  "}; 
        case HWC_TRANSFORM_ROT_270:
            return std::string{"ROT_270  "};
        default:
            return std::string{"UNKNOWN"};
            break;
    }
}

std::string blending(int blending_key)
{
    switch(blending_key)
    {
        case HWC_BLENDING_NONE:
            return std::string{"NONE    "};
        case HWC_BLENDING_PREMULT:
            return std::string{"PREMULT "};
        case HWC_BLENDING_COVERAGE:
            return std::string{"COVERAGE"};
        default:
            return std::string{"UNKNOWN"};
            break;
    }
}
}
void mga::HwcFormattedLogger::log_list_submitted_to_prepare(hwc_display_contents_1_t const& list)
{
    std::cout << "Before Prepare():" << std::endl
              << " # | pos {l,t,r,b}         | crop {l,t,r,b}        | transform | blending |" << std::endl;
    for(auto i = 0u; i < list.numHwLayers; i++)
    {
        std::cout << std::setw(2) << i << std::setw(0)
                  << " | {"
                  << std::setw(4) << list.hwLayers[i].displayFrame.left << std::setw(0) << ","
                  << std::setw(4) << list.hwLayers[i].displayFrame.top << std::setw(0) << "," 
                  << std::setw(4) << list.hwLayers[i].displayFrame.right << std::setw(0) << "," 
                  << std::setw(4) << list.hwLayers[i].displayFrame.bottom << std::setw(0)
                  << "} | {"
                  << std::setw(4) << list.hwLayers[i].sourceCrop.left << std::setw(0) << "," 
                  << std::setw(4) << list.hwLayers[i].sourceCrop.top << std::setw(0) << ","
                  << std::setw(4) << list.hwLayers[i].sourceCrop.right << std::setw(0) << ","
                  << std::setw(4) << list.hwLayers[i].sourceCrop.bottom << std::setw(0) 
                  << "} | "
                  << rotation(list.hwLayers[i].transform)
                  << " | "
                  << blending(list.hwLayers[i].blending)
                  << " |" << std::endl;
    }
}

void mga::HwcFormattedLogger::log_prepare_done(hwc_display_contents_1_t const& list)
{
    (void) list;
}

void mga::HwcFormattedLogger::log_set_list(hwc_display_contents_1_t const& list)
{
    (void) list;
}
