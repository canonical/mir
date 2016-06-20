/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/client/no_tls_future-inl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mcl = mir::client;

TEST(NoTLSFuture, and_then_calls_back_immediately_when_promise_is_already_fulfilled)
{
    mcl::NoTLSPromise<int> promise;
    promise.set_value(1);

    auto future = promise.get_future();
    bool callback_called{false};

    future.and_then([&callback_called](int) { callback_called = true;});
    EXPECT_TRUE(callback_called);
}

TEST(NoTLSFuture, and_then_calls_back_with_correct_value_when_promise_is_already_fulfilled)
{
    constexpr int expected{0xfeed};

    mcl::NoTLSPromise<int> promise;
    promise.set_value(expected);

    auto future = promise.get_future();
    future.and_then(
        [](int result)
        {
            using namespace testing;
            EXPECT_THAT(result, Eq(expected));
        });
}

TEST(NoTLSFuture, and_then_is_not_called_until_promise_is_fulfilled)
{
    constexpr char const* expected{"And then nothing turned itself inside out"};

    mcl::NoTLSPromise<std::string> promise;
    auto future = promise.get_future();

    bool called{false};
    future.and_then(
        [&called](std::string&& value)
        {
            using namespace testing;
            called = true;
            EXPECT_THAT(value, StrEq(expected));
        });

    EXPECT_FALSE(called);

    promise.set_value(expected);
    EXPECT_TRUE(called);
}

TEST(NoTLSFuture, or_else_is_called_when_promise_is_broken)
{
    auto promise = std::make_unique<mcl::NoTLSPromise<int>>();
    auto future = promise->get_future();

    bool called{false};
    future.or_else([&called](auto) { called = true; });

    EXPECT_FALSE(called);
    promise.reset();
    EXPECT_TRUE(called);
}
