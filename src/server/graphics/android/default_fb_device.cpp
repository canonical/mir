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

#include "mir/compositor/buffer.h"
#include "native_buffer_handle.h"

#include "default_fb_device.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mga=mir::graphics::android;
 
mga::DefaultFBDevice::DefaultFBDevice(std::shared_ptr<framebuffer_device_t> const& fbdev)
    : fb_device(fbdev)
{
}

void mga::DefaultFBDevice::post(std::shared_ptr<mir::compositor::Buffer> const& buffer)
{
    auto handle = buffer->native_buffer_handle();
    if (fb_device->post(fb_device.get(), handle->handle) != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error posting with fb device"));
    }
}
