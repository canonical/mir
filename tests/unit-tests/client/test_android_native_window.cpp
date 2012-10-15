/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir_client_surface.h"
#include "android/mir_native_window.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
namespace mcl=mir::client;

namespace
{
struct MockMirSurface : public mcl::ClientSurface
{
    MockMirSurface(MirSurfaceParameters params)
     : params(params)
    {
        using namespace testing;
        ON_CALL(*this, get_parameters())
            .WillByDefault(Return(params)); 
    }

    MOCK_CONST_METHOD0(get_parameters, MirSurfaceParameters());

    MirSurfaceParameters params;
};
}

class AndroidNativeWindowTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        surf_params.width = 530;
        surf_params.height = 715;
        surf_params.pixel_format = mir_pixel_format_rgba_8888; 

        mock_surface = std::make_shared<MockMirSurface>(surf_params);
    }

    MirSurfaceParameters surf_params;
    std::shared_ptr<MockMirSurface> mock_surface;
};

/* Query hook tests */
TEST_F(AndroidNativeWindowTest, native_window_query_hook_callable)
{
    ANativeWindow* anw;
    int value;
 
    anw = new mcl::MirNativeWindow(mock_surface.get());

    ASSERT_NE((int) anw->query, NULL);
    EXPECT_NO_THROW({
        anw->query(anw, NATIVE_WINDOW_WIDTH ,&value);
    });
    

    delete anw;
}

TEST_F(AndroidNativeWindowTest, native_window_width_query_hook)
{
    using namespace testing;
    ANativeWindow* anw;
    int value;
 
    anw = new mcl::MirNativeWindow(mock_surface.get());

    EXPECT_CALL(*mock_surface, get_parameters())
        .Times(1);

    auto rc = anw->query(anw, NATIVE_WINDOW_WIDTH ,&value);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(value, surf_params.width);

    delete anw;
}

TEST_F(AndroidNativeWindowTest, native_window_height_query_hook)
{
    using namespace testing;
    ANativeWindow* anw;
    int value;
 
    anw = new mcl::MirNativeWindow(mock_surface.get());
    EXPECT_CALL(*mock_surface, get_parameters())
        .Times(1);

    auto rc = anw->query(anw, NATIVE_WINDOW_HEIGHT ,&value);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(value, surf_params.height);

    delete anw;
}

TEST_F(AndroidNativeWindowTest, native_window_format_query_hook)
{
    using namespace testing;
    ANativeWindow* anw;
    int value;

    anw = new mcl::MirNativeWindow(mock_surface.get());
    EXPECT_CALL(*mock_surface, get_parameters())
        .Times(1);

    auto rc = anw->query(anw, NATIVE_WINDOW_FORMAT ,&value);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(value, HAL_PIXEL_FORMAT_RGBA_8888);

    delete anw;
}
