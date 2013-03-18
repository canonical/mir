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

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface.h"

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

static geom::Size const default_window_size = geom::Size{geom::Width{256},
                                                         geom::Height{256}};

struct StubInputApplicationHandle : public droidinput::InputApplicationHandle
{
    bool updateInfo() { return true; }
};

const int testing_server_fd = 2;

}

TEST(AndroidInputWindowHandle, update_info_uses_geometry_and_channel_from_surface)
{
    using namespace ::testing;

    mtd::MockSurface surface;

    EXPECT_CALL(surface, server_input_fd()).Times(1)
        .WillOnce(Return(testing_server_fd));
    
    mia::InputWindowHandle handle(new StubInputApplicationHandle(),
                                  mt::fake_shared(surface));
    
    // For now since we are just doing keyboard input we only need surface size, eventually will need
    // a position
    EXPECT_CALL(surface, size()).Times(1)
        .WillOnce(Return(default_window_size));
    
    EXPECT_TRUE(handle.updateInfo());
    
    // TODO Name
    
    // TODO: How do we avoid recreating a bunch of input hcannels when we move between communication channel and back
    auto info = handle.getInfo();
    EXPECT_EQ(testing_server_fd, info->inputChannel->getFd());
    
    EXPECT_EQ(default_window_size.height.as_uint32_t(), (uint32_t)(info->frameRight - info->frameLeft));
    EXPECT_EQ(default_window_size.height.as_uint32_t(), (uint32_t)(info->frameBottom - info->frameTop));
}
