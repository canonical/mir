/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/glib_main_loop.h"
#include "mir/main_loop.h"
#include "mir/shell/token_authority.h"
#include "mir/test/auto_unblock_thread.h"
#include "mir/test/spin_wait.h"
#include "mir/time/steady_clock.h"
#include <algorithm>
#include <chrono>
#include <gtest/gtest.h>
#include <iterator>
#include <memory>
#include <string>
#include <unistd.h>
#include <uuid.h>

struct TestTokenAuthority : testing::Test
{
    TestTokenAuthority() :
        clock{std::make_shared<mir::time::SteadyClock>()},
        main_loop{std::make_shared<mir::GLibMainLoop>(clock)},
        token_authority{std::shared_ptr<mir::MainLoop>(main_loop)},
        main_loop_thread{
            [this]()
            {
                main_loop->stop();
            },
            [this]()
            {
                main_loop->run();
            }}
    {
    }

    std::shared_ptr<mir::time::SteadyClock> const clock;
    std::shared_ptr<mir::MainLoop> const main_loop;
    mir::shell::TokenAuthority token_authority;
    mir::test::AutoUnblockThread main_loop_thread;
};

TEST_F(TestTokenAuthority, can_issue_tokens)
{
    auto const token = token_authority.issue_token({});

    // Make sure the generated token is a valid UUID
    uuid_t uuid;
    auto const token_string = std::string(token);
    auto ret = uuid_parse(token_string.c_str(), uuid);
    ASSERT_EQ(ret, 0);
}

TEST_F(TestTokenAuthority, tokens_are_revoked_in_time)
{
    using namespace std::chrono;

    auto constexpr revocation_time = milliseconds(3000);
    auto constexpr epsilon = milliseconds(50);

    bool revoked = false;
    auto const issuance_time = clock->now();

    auto const token = token_authority.issue_token(
        [&revoked, this, issuance_time, revocation_time, epsilon](auto&)
        {
            revoked = true;
            auto const delta_ms = duration_cast<milliseconds>(clock->now() - issuance_time);

            // The revocation time should be about 3 seconds, give or take a few ms
            ASSERT_LT(delta_ms - revocation_time, epsilon);
        });

    mir::test::spin_wait_for_condition_or_timeout(
        [&revoked]
        {
            return revoked;
        },
        revocation_time + epsilon);

    auto empty_token_opt = token_authority.get_token_for_string(std::string(token));

    // Test if the token authority has actually removed the token
    ASSERT_FALSE(empty_token_opt.has_value());
}

TEST_F(TestTokenAuthority, manual_revocation_works_correctly)
{
    bool revoked = false;
    auto const token = token_authority.issue_token(
        [&revoked](auto&)
        {
            revoked = true;
        });

    ASSERT_FALSE(revoked);

    token_authority.revoke_token(token);

    auto empty_token_opt = token_authority.get_token_for_string(std::string(token));
    ASSERT_FALSE(empty_token_opt.has_value());
    ASSERT_TRUE(revoked);
}

TEST_F(TestTokenAuthority, manual_revocation_doesnt_revoke_other_tokens)
{
    bool revoked1 = false, revoked2 = false;
    auto const token1 = token_authority.issue_token(
        [&revoked1](auto&)
        {
            revoked1 = true;
        });
    auto const token2 = token_authority.issue_token(
        [&revoked2](auto&)
        {
            revoked2 = true;
        });

    ASSERT_FALSE(revoked1);
    ASSERT_FALSE(revoked2);

    token_authority.revoke_token(token1);

    auto empty_token_opt = token_authority.get_token_for_string(std::string(token1));
    ASSERT_FALSE(empty_token_opt.has_value());
    ASSERT_TRUE(revoked1);
    ASSERT_FALSE(revoked2);
}

TEST_F(TestTokenAuthority, revoke_all_actually_revokes_all)
{
    std::vector<mir::shell::TokenAuthority::Token> tokens;
    std::ranges::generate_n(
        std::back_inserter(tokens),
        10,
        [this]()
        {
            return token_authority.issue_token({});
        });

    auto const token_valid = [this](auto const& token)
    {
        return token_authority.get_token_for_string(std::string(token)).has_value();
    };

    ASSERT_TRUE(std::ranges::all_of(tokens, token_valid));

    token_authority.revoke_all_tokens();

    ASSERT_TRUE(std::ranges::none_of(tokens, token_valid));
}

TEST_F(TestTokenAuthority, bogus_token_is_actually_bogus)
{
    auto const bogus_token = token_authority.get_bogus_token();

    // Make sure the generated bogus token is a valid UUID
    uuid_t uuid;
    auto const bogus_string = std::string(bogus_token);
    auto ret = uuid_parse(bogus_string.c_str(), uuid);
    ASSERT_EQ(ret, 0);

    // Mustn't be registered in the token authority as a genuine token
    ASSERT_FALSE(token_authority.get_token_for_string(bogus_string).has_value());
}
