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

#include "real_hwc_wrapper.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <sstream>

namespace mga=mir::graphics::android;

mga::RealHwcWrapper::RealHwcWrapper(std::shared_ptr<hwc_composer_device_1> const& hwc_device)
    : hwc_device(hwc_device)
{
}

void mga::RealHwcWrapper::prepare(hwc_display_contents_1_t& display_list) const
{
    //note, although we only have a primary display right now,
    //      set the external and virtual displays to null as some drivers check for that
    hwc_display_contents_1_t* displays[num_displays] {&display_list, nullptr, nullptr};
    for(auto i = 0u; i < display_list.numHwLayers; i++)
    {
        printf("\tPREPARE handle %i:   %x\n",i, (int) display_list.hwLayers[i].handle);
    }
    if (auto rc = hwc_device->prepare(hwc_device.get(), 1, displays))
    {
        std::stringstream ss;
        ss << "error during hwc prepare(). rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
}

void mga::RealHwcWrapper::set(hwc_display_contents_1_t& display_list) const
{
    hwc_display_contents_1_t* displays[num_displays] {&display_list, nullptr, nullptr};
    printf("num: %i\n", display_list.numHwLayers);
    for(auto i = 0u; i < display_list.numHwLayers; i++)
    {
        printf("layer\n");
        printf("\tcomptype: %x\n", display_list.hwLayers[i].compositionType);
        printf("\thandle:   %x\n", (int) display_list.hwLayers[i].handle);
        printf("\tspos:   %i %i %i %i\n", display_list.hwLayers[i].displayFrame.top,
                                          display_list.hwLayers[i].displayFrame.bottom,
                                          display_list.hwLayers[i].displayFrame.left,
                                          display_list.hwLayers[i].displayFrame.right);
        printf("\tcrop:   %i %i %i %i\n", display_list.hwLayers[i].sourceCrop.top,
                                          display_list.hwLayers[i].sourceCrop.bottom,
                                          display_list.hwLayers[i].sourceCrop.left,
                                          display_list.hwLayers[i].sourceCrop.right);
        printf("\tvrs :   %i %i %i %i\n", display_list.hwLayers[i].visibleRegionScreen.rects[0].top,
                                          display_list.hwLayers[i].visibleRegionScreen.rects[0].bottom,
                                          display_list.hwLayers[i].visibleRegionScreen.rects[0].left,
                                          display_list.hwLayers[i].visibleRegionScreen.rects[0].right);

        printf("\tacquire: %i\n", (int) display_list.hwLayers[i].acquireFenceFd);
        printf("\trelease: %i\n", (int) display_list.hwLayers[i].releaseFenceFd);
    }
    if (auto rc = hwc_device->set(hwc_device.get(), 1, displays))
    {
        std::stringstream ss;
        ss << "error during hwc prepare(). rc = " << std::hex << rc;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
}
