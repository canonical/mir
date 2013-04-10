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

#include "mir_test_doubles/mock_fb_device.h"
#include "mir_test_doubles/stub_buffer.h"
#include "src/server/graphics/android/default_fb_device.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mtd=mir::test::doubles;
namespace mga=mir::graphics::android;

struct FBDevice : public ::testing::Test
{
    virtual void SetUp()
    {
        fb_hal_mock = std::make_shared<mtd::MockFBHalDevice>(); 
    }

    std::shared_ptr<mtd::MockFBHalDevice> fb_hal_mock;
};

TEST_F(FBDevice, post_ok)
{
    using namespace testing;
    mga::DefaultFBDevice fbdev(fb_hal_mock);

    auto stub_buffer = std::make_shared<mtd::StubBuffer>();
    EXPECT_CALL(*fb_hal_mock, post_interface(_,_))
        .Times(1);

    fbdev.post(stub_buffer); 
}

TEST_F(FBDevice, post_fail)
{
    using namespace testing;
    mga::DefaultFBDevice fbdev(fb_hal_mock);

    auto stub_buffer = std::make_shared<mtd::StubBuffer>();
    EXPECT_CALL(*fb_hal_mock, post_interface(_,_))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        fbdev.post(stub_buffer);
    }, std::runtime_error); 
}
