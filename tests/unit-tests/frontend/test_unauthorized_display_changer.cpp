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

#include "src/server/frontend/unauthorized_display_changer.h"

#include "mir/test/doubles/mock_display_changer.h"
#include "mir/test/doubles/null_display_configuration.h"

#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;

struct UnauthorizedDisplayChangerTest : public ::testing::Test
{
    mtd::MockDisplayChanger underlying_changer;
    std::shared_ptr<mtd::NullDisplayConfiguration> const conf{
        std::make_shared<mtd::NullDisplayConfiguration>()};
    mf::UnauthorizedDisplayChanger changer{mt::fake_shared(underlying_changer)};
};

TEST_F(UnauthorizedDisplayChangerTest, disallows_configure_display_by_default)
{
    EXPECT_THROW({
        changer.configure(std::shared_ptr<mf::Session>(), conf);
    }, std::runtime_error);
}

TEST_F(UnauthorizedDisplayChangerTest, disallows_set_base_configuration_by_default)
{
    EXPECT_THROW({
        changer.set_base_configuration(conf);
    }, std::runtime_error);
}

TEST_F(UnauthorizedDisplayChangerTest, can_allow_configure_display)
{
    using namespace testing;

    EXPECT_CALL(underlying_changer, configure(_, _)).Times(1);

    changer.allow_configure_display();
    changer.configure(std::shared_ptr<mf::Session>(), conf);
}

TEST_F(UnauthorizedDisplayChangerTest, can_allow_set_base_display_configuration)
{
    using namespace testing;

    EXPECT_CALL(underlying_changer, mock_set_base_configuration(_)).Times(1);

    changer.allow_set_base_configuration();
    changer.set_base_configuration(conf);
}

TEST_F(UnauthorizedDisplayChangerTest, access_config)
{
    using namespace testing;

    EXPECT_CALL(underlying_changer, base_configuration())
        .WillOnce(Return(conf));

    auto returned_conf = changer.base_configuration();

    EXPECT_EQ(conf, returned_conf);
}
