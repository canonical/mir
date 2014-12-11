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

#include "src/platform/graphics/android/display_buffer_builder.h"
#include "src/platform/graphics/android/hwc_configuration.h"
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
    bool post_renderables_if_optimizable(graphics::RenderableList const&) { return false; }
    MirOrientation orientation() const override { return mir_orientation_normal; }
    bool uses_alpha() const override { return false; };
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

struct MockHwcConfiguration : public graphics::android::HwcConfiguration
{
    MOCK_METHOD2(power_mode, void(graphics::android::DisplayName, MirPowerMode));
};

struct StubDisplayBuilder : public graphics::android::DisplayBufferBuilder
{
    StubDisplayBuilder(geometry::Size sz)
        : sz(sz),
          next_mock_display_config{new MockHwcConfiguration}
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
        graphics::GLProgramFactory const&,
        graphics::android::GLContext const&)
    {
        return std::unique_ptr<graphics::android::ConfigurableDisplayBuffer>(
                new StubConfigurableDisplayBuffer(geometry::Rectangle{{0,0},sz}));
    }

    std::unique_ptr<graphics::android::HwcConfiguration> create_hwc_config()
    {
        auto config = std::unique_ptr<MockHwcConfiguration>(new MockHwcConfiguration);
        std::swap(config, next_mock_display_config);
        return std::move(config);
    }

    void with_next_config(std::function<void(MockHwcConfiguration&)> const& config)
    {
        config(*next_mock_display_config);
    }
private: 
    geometry::Size sz;
    std::unique_ptr<MockHwcConfiguration> next_mock_display_config;
};
}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_BUILDER_H_ */
