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

#include "src/server/graphics/android/internal_client_window.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_swapper.h"

#include <gtest/gtest.h>

namespace mc=mir::compositor;
namespace mtd=mir::test::doubles;
namespace mga=mir::graphics::android;
 
struct InternalClientWindow : public ::testing::Test
{
};

TEST_F(InternalClientWindow, driver_requests_buffer)
{
    using namespace testing;
    auto stub_anw = std::make_shared<ANativeWindowBuffer>();
    auto mock_buffer = std::make_shared<mtd::MockBuffer>();
    auto mock_swapper = std::unique_ptr<mtd::MockSwapper>(new mtd::MockSwapper());
    EXPECT_CALL(*mock_buffer, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(stub_anw));
    EXPECT_CALL(*mock_swapper, client_acquire())
        .Times(1)
        .WillOnce(Return(mock_buffer));

    mga::InternalClientWindow interpreter(std::move(mock_swapper));
    auto test_buffer = interpreter.driver_requests_buffer();
    EXPECT_EQ(stub_anw.get(), test_buffer); 
}

TEST_F(InternalClientWindow, driver_returns_buffer)
{
    using namespace testing;
    auto stub_anw = std::make_shared<ANativeWindowBuffer>();
    auto mock_buffer = std::make_shared<mtd::MockBuffer>();
    auto mock_swapper = std::unique_ptr<mtd::MockSwapper>(new mtd::MockSwapper());
    EXPECT_CALL(*mock_buffer, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(stub_anw));
    EXPECT_CALL(*mock_swapper, client_acquire())
        .Times(1)
        .WillOnce(Return(mock_buffer));

    mga::InternalClientWindow interpreter(std::move(mock_swapper));
    auto test_bufferptr = interpreter.driver_requests_buffer();

    /* end setup */
    std::shared_ptr<mc::Buffer> tmp = mock_buffer;
    EXPECT_CALL(*mock_swapper, client_release(tmp))
        .Times(1);

    std::shared_ptr<mga::SyncObject> fake_sync;
    interpreter.driver_returns_buffer(test_bufferptr, fake_sync);
}

TEST_F(InternalClientWindow, driver_requests_buffer_ownership)
{
    using namespace testing;
    auto stub_anw = std::make_shared<ANativeWindowBuffer>();
    auto mock_buffer = std::make_shared<mtd::MockBuffer>();
    auto mock_swapper = std::unique_ptr<mtd::MockSwapper>(new mtd::MockSwapper());
    EXPECT_CALL(*mock_buffer, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(stub_anw));
    EXPECT_CALL(*mock_swapper, client_acquire())
        .Times(1)
        .WillOnce(Return(mock_buffer));

    mga::InternalClientWindow interpreter(std::move(mock_swapper));

    auto use_count_before = mock_buffer.use_count();
    auto test_anwb = interpreter.driver_requests_buffer();
    EXPECT_EQ(use_count_before + 1, mock_buffer.use_count());

    std::shared_ptr<mga::SyncObject> fake_sync;
    interpreter.driver_returns_buffer(test_anwb, fake_sync);
    EXPECT_EQ(use_count_before, mock_buffer.use_count());
}
