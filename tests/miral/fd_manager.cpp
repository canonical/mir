/*
 * Copyright © 2022 Canonical Ltd.
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

TEST(MockMainLoop, dropping_fd_handle_before_server_started_removes_fd_from_backlog)
{
    auto manager = std::make_shared<miral::FdManager>();
    auto main_loop = std::make_shared<MockMainLoop>();

    main_loop->run();

    manager->set_weak_main_loop(main_loop);

    auto fd = mir::Fd{42};

    {
        auto handle = manager->register_handler(fd, [this](int) { std::function<void(int)>(); });
        EXPECT_CALL(*main_loop.get(), unregister_fd_handler(_));
    }
}
