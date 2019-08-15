/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/server/frontend/authorizing_display_changer.h"

#include "mir/test/doubles/mock_display_changer.h"
#include "mir/test/doubles/null_display_configuration.h"

#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;
namespace ms = mir::scene;

struct AuthorizingDisplayChangerTest : public ::testing::TestWithParam<std::tuple<bool, bool>>
{
    testing::NiceMock<mtd::MockDisplayChanger> underlying_changer;
    std::shared_ptr<mtd::NullDisplayConfiguration> const conf{
        std::make_shared<mtd::NullDisplayConfiguration>()};
    mf::AuthorizingDisplayChanger changer{
        mt::fake_shared(underlying_changer),
        std::get<0>(GetParam()),
        std::get<1>(GetParam())};
};


TEST_P(AuthorizingDisplayChangerTest, configure_throws_only_when_configure_is_disallowed)
{
    bool const configure_allowed = std::get<0>(GetParam());

    if (configure_allowed)
    {
        EXPECT_NO_THROW({ changer.configure(std::shared_ptr<ms::Session>(), conf); });
    }
    else
    {
        EXPECT_THROW(
            {
                changer.configure(std::shared_ptr<ms::Session>(), conf);
            },
            std::runtime_error);
    }
}

TEST_P(AuthorizingDisplayChangerTest, set_base_configuration_throws_only_when_disallowed)
{
    bool const base_configure_allowed = std::get<1>(GetParam());

    if (base_configure_allowed)
    {
        EXPECT_NO_THROW({ changer.set_base_configuration(conf); });
    }
    else
    {
        EXPECT_THROW(
            {
                changer.set_base_configuration(conf);
            },
            std::runtime_error);
    }
}

TEST_P(AuthorizingDisplayChangerTest, preview_base_configuration_throws_only_when_disallowed)
{
    bool const base_configure_allowed = std::get<1>(GetParam());
    std::weak_ptr<ms::Session> null_session;
    std::chrono::seconds timeout;

    if (base_configure_allowed)
    {
        EXPECT_NO_THROW({ changer.preview_base_configuration(null_session, conf, timeout); });
    }
    else
    {
        EXPECT_THROW(
            {
                changer.preview_base_configuration(null_session, conf, timeout);
            },
            std::runtime_error);
    }
}

TEST_P(AuthorizingDisplayChangerTest, only_calls_configure_if_authorized)
{
    using namespace testing;

    bool const configure_allowed = std::get<0>(GetParam());

    EXPECT_CALL(underlying_changer, configure(_, _)).Times(configure_allowed);

    try
    {
        changer.configure(std::shared_ptr<ms::Session>(), conf);
    }
    catch (...)
    {
    }
}

TEST_P(AuthorizingDisplayChangerTest, only_calls_set_base_configuration_if_authorized)
{
    using namespace testing;

    bool const base_configure_allowed = std::get<1>(GetParam());

    EXPECT_CALL(underlying_changer, mock_set_base_configuration(_)).Times(base_configure_allowed);

    try
    {
        changer.set_base_configuration(conf);
    }
    catch (...)
    {
    }
}

TEST_P(AuthorizingDisplayChangerTest, only_calls_preview_base_configuration_if_authorized)
{
    using namespace testing;

    bool const base_configure_allowed = std::get<1>(GetParam());

    EXPECT_CALL(underlying_changer, preview_base_configuration(_,_,_)).Times(base_configure_allowed);

    try
    {
        changer.preview_base_configuration(
            std::weak_ptr<ms::Session>{},
            conf,
            std::chrono::seconds{1});
    }
    catch (...)
    {
    }
}

TEST_P(AuthorizingDisplayChangerTest, can_always_access_config)
{
    using namespace testing;

    EXPECT_CALL(underlying_changer, base_configuration())
        .WillOnce(Return(conf));

    auto returned_conf = changer.base_configuration();

    EXPECT_EQ(conf, returned_conf);
}

INSTANTIATE_TEST_CASE_P(Authorization,
    AuthorizingDisplayChangerTest,
    testing::Combine(testing::Bool(), testing::Bool()));
