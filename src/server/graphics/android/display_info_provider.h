/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_GRAPHICS_ANDROID_DISPLAY_INFO_PROVIDER_H_
#define MIR_GRAPHICS_ANDROID_DISPLAY_INFO_PROVIDER_H_

#include "mir/geometry/size.h"
#include "mir/geometry/pixel_format.h"
#include <memory>

namespace mir
{
namespace compositor
{
class Buffer;
}
namespace graphics
{
namespace android
{

class DisplayInfoProvider
{
public:
    virtual ~DisplayInfoProvider() {}

    virtual geometry::Size display_size() = 0; 
    virtual geometry::PixelFormat display_format() = 0; 
    virtual unsigned int number_of_framebuffers_available() = 0;
    virtual void set_next_frontbuffer(std::shared_ptr<compositor::Buffer> const& buffer) = 0;

protected:
    DisplayInfoProvider() = default;
    DisplayInfoProvider& operator=(DisplayInfoProvider const&) = delete;
    DisplayInfoProvider(DisplayInfoProvider const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DISPLAY_INFO_PROVIDER_H_ */
