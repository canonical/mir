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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_DISPLAY_BUILDER_H_
#define MIR_GRAPHICS_ANDROID_DISPLAY_BUILDER_H_

#include "mir/graphics/display_buffer.h"
#include "mir_toolkit/common.h"
#include <memory>

namespace mir
{
namespace graphics
{
namespace android
{
class DisplayDevice;
class GLContext;

class DisplayBuilder
{
public:
    virtual ~DisplayBuilder() = default;

    virtual MirPixelFormat display_format() = 0;
    virtual std::shared_ptr<DisplayDevice> create_display_device() = 0;
    virtual std::unique_ptr<graphics::DisplayBuffer> create_display_buffer(
        std::shared_ptr<DisplayDevice> const& display_device,
        GLContext const& gl_context) = 0;

protected:
    DisplayBuilder() = default;
    DisplayBuilder(DisplayBuilder const&) = delete;
    DisplayBuilder& operator=(DisplayBuilder const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DISPLAY_BUILDER_H_ */
