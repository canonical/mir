/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "android_format_conversion-inl.h"
#include "fb_info.h"

namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::FBInfo::FBInfo(std::shared_ptr<framebuffer_device_t> const& fb_device)
 : fb_device(fb_device)
{
}

geom::Size mga::FBInfo::display_size() const
{
    return {fb_device->width, fb_device->height};
} 

geom::PixelFormat mga::FBInfo::display_format() const
{
    return to_mir_format(fb_device->format);
}

unsigned int mga::FBInfo::number_of_framebuffers_available() const
{
    auto fb_num = static_cast<unsigned int>(fb_device->numFramebuffers);
    return std::max(2u, fb_num);
}
