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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "hwc_common_device.h"

namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::HWCCommonDevice::HWCCommonDevice()
{
}

mga::HWCCommonDevice::~HWCCommonDevice() noexcept
{
}

geom::PixelFormat mga::HWCCommonDevice::display_format() const
{
    return geom::PixelFormat::abgr_8888;
}
unsigned int mga::HWCCommonDevice::number_of_framebuffers_available() const
{
    return 2u;
}
void mga::HWCCommonDevice::set_next_frontbuffer(std::shared_ptr<AndroidBuffer> const& /*buffer*/)
{
}
void mga::HWCCommonDevice::wait_for_vsync()
{
}
void mga::HWCCommonDevice::notify_vsync()
{
}
