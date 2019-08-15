/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "src/server/frontend/authorizing_input_config_changer.h"

#include "mir/test/doubles/mock_input_config_changer.h"

#include "mir/test/fake_shared.h"
#include "mir/input/mir_input_config.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;
namespace ms = mir::scene;

struct AuthorizingInputConfigChanger : public ::testing::TestWithParam<std::tuple<bool, bool>>
{
    testing::NiceMock<mtd::MockInputConfigurationChanger> underlying_changer;
    MirInputConfig config;
    mf::AuthorizingInputConfigChanger changer{
        mt::fake_shared(underlying_changer),
        std::get<0>(GetParam()),
        std::get<1>(GetParam())};
};


TEST_P(AuthorizingInputConfigChanger, configure_throws_only_when_configure_is_disallowed)
{
    bool const configure_allowed = std::get<0>(GetParam());

    if (configure_allowed)
    {
        EXPECT_NO_THROW({ changer.configure(std::shared_ptr<ms::Session>(), MirInputConfig{}); });
    }
    else
    {
        EXPECT_THROW(
            {
                changer.configure(std::shared_ptr<ms::Session>(), MirInputConfig{});
            },
            std::runtime_error);
    }
}

TEST_P(AuthorizingInputConfigChanger, set_base_configuration_throws_only_when_disallowed)
{
    bool const base_configure_allowed = std::get<1>(GetParam());

    if (base_configure_allowed)
    {
        EXPECT_NO_THROW({ changer.set_base_configuration(MirInputConfig{}); });
    }
    else
    {
        EXPECT_THROW(
            {
                changer.set_base_configuration(MirInputConfig{});
            },
            std::runtime_error);
    }
}


TEST_P(AuthorizingInputConfigChanger, can_always_access_config)
{
    using namespace testing;

    EXPECT_CALL(underlying_changer, base_configuration())
        .WillOnce(Return(MirInputConfig{}));

    auto returned_conf = changer.base_configuration();

    EXPECT_EQ(config, returned_conf);
}

INSTANTIATE_TEST_CASE_P(Authorization,
    AuthorizingInputConfigChanger,
    testing::Combine(testing::Bool(), testing::Bool()));
