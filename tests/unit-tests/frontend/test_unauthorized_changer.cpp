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

#include "mir/frontend/unauthorized_display_changer.h"
#include "mir_test_doubles/mock_display_changer.h"
#include <gtest/gtest.h>

struct UnauthorizedDisplayChangerTest : public ::testing::Test
{
    MockDisplayChanger underlying_changer;
};

TEST_F(UnauthorizedDisplayChangerTest, change_attempt)
{
    UnauthorizedDisplayChanger changer(mt::fake_shared(underlying_changer));
    EXPECT_THROW({
        changer.configure(nullptr, nullptr);
    }, std::runtime_error);
}

TEST_F(UnauthorizedDisplayChangerTest, change_attempt)
{
    using namespace testing;

    NullDisplayConfiguration conf;
    EXPECT_CALL(underlying_changer, active_configuration())
        .Times(1)
        .WillOnce(Return(mt::fake_shared(conf));

    UnauthorizedDisplayChanger changer(mt::fake_shared(underlying_changer));

    auto returned_conf = changer.active_configuration();
    EXPECT_EQ(&conf, returned_conf.get());
}
