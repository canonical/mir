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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "hwc10_device.h"

namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::HWC10Device::HWC10Device(std::shared_ptr<hwc_composer_device_1> const& /*hwc_device*/,
                              std::shared_ptr<HWCLayerOrganizer> const& /*organizer*/,
                              std::shared_ptr<FBDevice> const& /*fbdev*/)
{
}

mga::HWC10Device::~HWC10Device() noexcept
{
}

geom::Size mga::HWC10Device::display_size() const
{
    return geom::Size{geom::Width{43}, geom::Height{22}};
}

geom::PixelFormat mga::HWC10Device::display_format() const
{
    return geom::PixelFormat::abgr_8888;
}

unsigned int mga::HWC10Device::number_of_framebuffers_available() const
{
    //note: the default for hwc devices is 2 framebuffers. However, the api allows for the hwc can give us a hint
    //      to triple buffer. Taking this hint is currently not supported
    return 2u;
}
 
void mga::HWC10Device::notify_vsync()
{
}

void mga::HWC10Device::wait_for_vsync()
{
}

void mga::HWC10Device::set_next_frontbuffer(std::shared_ptr<mga::AndroidBuffer> const& /*buffer*/)
{
}

void mga::HWC10Device::commit_frame()
{
}
