/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fd_manager.h"

#include <mir/test/doubles/mock_main_loop.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

using namespace mir::test::doubles;

struct FdManager : public testing::Test
{
    std::shared_ptr<miral::FdManager> const manager = std::make_shared<miral::FdManager>();
    std::shared_ptr<NiceMock<MockMainLoop>> const main_loop = std::make_shared<NiceMock<MockMainLoop>>();
    mir::Fd fd = mir::Fd{42};

    void attach_main_loop()
    {
        manager->set_main_loop(main_loop);
    }
};

TEST_F(FdManager, dropping_fd_handle_after_main_loop_created_unregisters_handler)
{
    attach_main_loop();

    {
        auto const handle = manager->register_handler(fd, [](int) { std::function<void(int)>(); });
        EXPECT_CALL(*main_loop.get(), unregister_fd_handler(_));
    }
}

TEST_F(FdManager, dropping_fd_handle_before_main_loop_created_does_not_register_handler)
{
    EXPECT_CALL(*main_loop.get(), register_fd_handler(_, _, _))
        .Times(0);
    auto const handle = manager->register_handler(fd, [](int) { std::function<void(int)>(); });
}

TEST_F(FdManager, dropping_fd_handle_before_main_loop_created_does_not_register_handler_after_main_loop_created)
{
    {
        auto const handle = manager->register_handler(fd, [](int) { std::function<void(int)>(); });
    }

    EXPECT_CALL(*main_loop.get(), register_fd_handler(_, _, _))
        .Times(0);

    attach_main_loop();
}

TEST_F(FdManager, register_handler_after_main_loop_created_registers_fd_handler)
{
    attach_main_loop();

    EXPECT_CALL(*main_loop.get(), register_fd_handler(_, _, _));
    auto const handle = manager->register_handler(fd, [](int) { std::function<void(int)>(); });
}

TEST_F(FdManager, register_handler_before_main_loop_created_registers_fd_handler_after_main_loop_created)
{
    auto const handle = manager->register_handler(fd, [](int) { std::function<void(int)>(); });

    EXPECT_CALL(*main_loop.get(), register_fd_handler(_, _, _))
        .Times(1);

    attach_main_loop();
}
