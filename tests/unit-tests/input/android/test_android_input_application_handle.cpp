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

#include "src/server/input/android/android_input_application_handle.h"

#include "mir/input/surface_target.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <limits.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace geom = mir::geometry;

namespace
{
struct MockSurfaceHandle : public mi::SurfaceTarget
{
    MOCK_CONST_METHOD0(server_input_fd, int());
    MOCK_CONST_METHOD0(top_left, geom::Point());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(name, std::string const&());
};
}


TEST(AndroidInputApplicationHandle, takes_name_from_surface_and_specifies_max_timeout)
{
    using namespace ::testing;
    std::string testing_surface_name = "Cats";
    auto surface = std::make_shared<MockSurfaceHandle>();

    EXPECT_CALL(*surface, name()).Times(AtLeast(1))
        .WillRepeatedly(ReturnRef(testing_surface_name));
    mia::InputApplicationHandle application_handle(surface);
    EXPECT_TRUE(application_handle.updateInfo());
    auto info = application_handle.getInfo();
    EXPECT_EQ(INT_MAX, info->dispatchingTimeout);
    EXPECT_EQ(droidinput::String8(testing_surface_name.c_str()), info->name);
}

TEST(AndroidInputApplicationHandle, does_not_own_surface)
{
    using namespace ::testing;
    std::string const testing_surface_name = "Let it Grow";
    
    auto surface = std::make_shared<MockSurfaceHandle>();
    EXPECT_CALL(*surface, name()).Times(AtLeast(1))
        .WillRepeatedly(ReturnRef(testing_surface_name));
    
    mia::InputApplicationHandle application_handle(surface);
    EXPECT_TRUE(application_handle.updateInfo());
    surface.reset();
    EXPECT_FALSE(application_handle.updateInfo());

}

