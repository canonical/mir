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

#include "default_fb_device.h"

namespace mga=mir::graphics::android;
 
mga::DefaultFBDevice::DefaultFBDevice(std::shared_ptr<framebuffer_device_t> const&)
{
}

void mga::DefaultFBDevice::post(std::shared_ptr<mir::compositor::Buffer> const& /*buffer*/)
{
}
