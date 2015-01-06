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

#include "src/platforms/android/display_buffer_builder.h"
#include "src/platforms/android/configurable_display_buffer.h"
#include "src/platforms/android/hwc_configuration.h"
#include "mock_display_device.h"
#include "mock_framebuffer_bundle.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockHwcConfiguration : public graphics::android::HwcConfiguration
{
    MOCK_METHOD2(power_mode, void(graphics::android::DisplayName, MirPowerMode));
    MOCK_METHOD1(active_attribs_for, graphics::android::DisplayAttribs(graphics::android::DisplayName));
};

struct StubDisplayBuilder : public graphics::android::DisplayBufferBuilder
{
    StubDisplayBuilder(geometry::Size sz)
        : sz(sz),
          mock_config{new testing::NiceMock<MockHwcConfiguration>()}
    {
    }

    StubDisplayBuilder()
        : StubDisplayBuilder(geometry::Size{0,0})
    {
    }

    std::unique_ptr<graphics::android::FramebufferBundle> create_framebuffers(graphics::android::DisplayAttribs const&) override
    {
        return std::unique_ptr<graphics::android::FramebufferBundle>(new MockFBBundle());
    }

    std::unique_ptr<graphics::android::DisplayDevice> create_display_device() override
    {
        return std::unique_ptr<graphics::android::DisplayDevice>(new MockDisplayDevice());
    }

    std::unique_ptr<graphics::android::HwcConfiguration> create_hwc_configuration() override
    {
        auto config = std::unique_ptr<MockHwcConfiguration>(new testing::NiceMock<MockHwcConfiguration>());
        std::swap(config, mock_config);
        return std::move(config);
    }
    
    void with_next_config(std::function<void(MockHwcConfiguration& mock_config)> const& fn)
    {
        fn(*mock_config); 
    }

    geometry::Size sz;
    std::unique_ptr<MockHwcConfiguration> mock_config;
};
}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_BUILDER_H_ */
