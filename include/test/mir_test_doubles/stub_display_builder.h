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

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_BUILDER_H_
#define MIR_TEST_DOUBLES_STUB_DISPLAY_BUILDER_H_

#include "src/platform/graphics/android/display_builder.h"
#include "src/platform/graphics/android/configurable_display_buffer.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct StubConfigurableDisplayBuffer : public graphics::android::ConfigurableDisplayBuffer
{
    StubConfigurableDisplayBuffer(geometry::Rectangle rect)
     : rect(rect)
    {
    }

    geometry::Rectangle view_area() const { return rect; }
    void make_current() {}
    void release_current() {}
    void post_update() {}
    bool can_bypass() const override { return false; }
    void render_and_post_update(
        std::list<std::shared_ptr<graphics::Renderable>> const&,
        std::function<void(graphics::Renderable const&)> const&) {}
    MirOrientation orientation() const override { return mir_orientation_normal; }
    void configure(graphics::DisplayConfigurationOutput const&) {} 
    graphics::DisplayConfigurationOutput configuration() const
    {
        return graphics::DisplayConfigurationOutput{
                   graphics::DisplayConfigurationOutputId{1},
                   graphics::DisplayConfigurationCardId{0},
                   graphics::DisplayConfigurationOutputType::vga,
                   {}, {}, 0, {}, false, false, {}, 0, mir_pixel_format_abgr_8888, 
                   mir_power_mode_off,
                   mir_orientation_normal};
    }
private:
    geometry::Rectangle rect;
};

struct StubDisplayBuilder : public graphics::android::DisplayBuilder
{
    StubDisplayBuilder(geometry::Size sz)
        : sz(sz)
    {
    }

    StubDisplayBuilder()
        : StubDisplayBuilder(geometry::Size{0,0})
    {
    }

    MirPixelFormat display_format()
    {
        return mir_pixel_format_abgr_8888;
    }

    std::unique_ptr<graphics::android::ConfigurableDisplayBuffer> create_display_buffer(
        graphics::android::GLContext const&)
    {
        return std::unique_ptr<graphics::android::ConfigurableDisplayBuffer>(
                new StubConfigurableDisplayBuffer(geometry::Rectangle{{0,0},sz}));
    }
    
    geometry::Size sz;
};
}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_BUILDER_H_ */
