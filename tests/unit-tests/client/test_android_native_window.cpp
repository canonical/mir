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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_client_surface.h"
#include "android/mir_native_window.h"

namespace mcl=mir::client;

namespace
{
struct MockMirSurface : public mcl::ClientSurface
{
    MOCK_CONST_METHOD0(get_parameters, MirSurfaceParameters());
};
}

class AndroidNativeWindowTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }
};

TEST_F(AndroidNativeWindowTest, native_window_query_hook_callable)
{
    MockMirSurface mock_surface;
    ANativeWindow* anw;
    int value;
 
    anw = new mcl::MirNativeWindow(&mock_surface);

    ASSERT_NE((int) anw->query, NULL);
    EXPECT_NO_THROW({
        anw->query(anw, NATIVE_WINDOW_WIDTH ,&value);
    });
    

    delete anw;
}

TEST_F(AndroidNativeWindowTest, native_window_query_hook)
{
    MockMirSurface mock_surface;
    ANativeWindow* anw;
    int value;
 
    anw = new mcl::MirNativeWindow(&mock_surface);

    EXPECT_CALL(mock_surface, get_parameters())
        .Times(1);

    anw->query(anw, NATIVE_WINDOW_WIDTH ,&value);

    delete anw;
}
