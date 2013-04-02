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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/server/input/android/android_input_window_handle.h"

#include "mir/frontend/surface.h"
#include "mir/geometry/size.h"
#include "mir/input/input_channel.h"
#include "mir/input/surface_target.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/stub_surface_builder.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mc = mir::compositor;
namespace mi = mir::input;
namespace mia = mi::android;
namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{

struct StubInputApplicationHandle : public droidinput::InputApplicationHandle
{
    bool updateInfo() { return true; }
};

struct MockSurfaceTarget : public mi::SurfaceTarget
{
    MOCK_CONST_METHOD0(server_input_fd, int());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(name, std::string());
};

}

TEST(AndroidInputWindowHandle, update_info_uses_geometry_and_channel_from_surface)
{
    using namespace ::testing;

    geom::Size const default_surface_size = geom::Size{geom::Width{256},
                                                      geom::Height{256}};
    std::string const testing_surface_name = "Test";
    int const testing_server_fd = 2;

    MockSurfaceTarget surface;

    EXPECT_CALL(surface, server_input_fd()).Times(1)
        .WillOnce(Return(testing_server_fd));
    // For now since we are just doing keyboard input we only need surface size,
    // for touch/pointer events we will need a position
    EXPECT_CALL(surface, size()).Times(1)
        .WillOnce(Return(default_surface_size));
    EXPECT_CALL(surface, name()).Times(1)
        .WillOnce(Return(testing_surface_name));

    mia::InputWindowHandle handle(new StubInputApplicationHandle(),
                                  mt::fake_shared(surface));

    auto info = handle.getInfo();

    EXPECT_EQ(testing_surface_name, info->name);

    EXPECT_EQ(testing_server_fd, info->inputChannel->getFd());

    EXPECT_EQ(default_surface_size.height.as_uint32_t(), (uint32_t)(info->frameRight - info->frameLeft));
    EXPECT_EQ(default_surface_size.height.as_uint32_t(), (uint32_t)(info->frameBottom - info->frameTop));
}
