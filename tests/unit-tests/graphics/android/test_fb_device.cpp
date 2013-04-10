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
#include "mir_test_doubles/mock_buffer.h"
#include "src/server/graphics/android/default_fb_device.h"
#include "src/server/graphics/android/native_buffer_handle.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mtd=mir::test::doubles;
namespace mga=mir::graphics::android;
namespace mc=mir::compositor;

struct FBDevice : public ::testing::Test
{
    virtual void SetUp()
    {
        using namespace testing;
        fb_hal_mock = std::make_shared<mtd::MockFBHalDevice>(); 
        mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>();

        dummy_buffer = std::make_shared<mc::NativeBufferHandle>();
        dummy_buffer->handle = (buffer_handle_t) 0x4893;
        ON_CALL(*mock_buffer, native_buffer_handle())
            .WillByDefault(Return(dummy_buffer));
    }

    std::shared_ptr<mtd::MockFBHalDevice> fb_hal_mock;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    std::shared_ptr<mc::NativeBufferHandle> dummy_buffer;
};

TEST_F(FBDevice, post_ok)
{
    using namespace testing;
    mga::DefaultFBDevice fbdev(fb_hal_mock);

    EXPECT_CALL(*fb_hal_mock, post_interface(fb_hal_mock.get(),dummy_buffer->handle))
        .Times(1);

    fbdev.post(mock_buffer); 
}

TEST_F(FBDevice, post_fail)
{
    using namespace testing;
    mga::DefaultFBDevice fbdev(fb_hal_mock);

    EXPECT_CALL(*fb_hal_mock, post_interface(fb_hal_mock.get(),dummy_buffer->handle))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        fbdev.post(mock_buffer);
    }, std::runtime_error); 
}
