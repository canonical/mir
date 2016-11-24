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

#include "mir/test/auto_unblock_thread.h"
#include "mir/test/signal.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mcl = mir::client;
namespace mt = mir::test;

TEST(NoTLSFuture, then_calls_back_immediately_when_promise_is_already_fulfilled)
{
    mcl::NoTLSPromise<int> promise;
    promise.set_value(1);

    auto future = promise.get_future();
    bool callback_called{false};

    auto result = future.then(
        [&callback_called](mcl::NoTLSFuture<int>&& done)
        {
            done.get();
            callback_called = true;
        });
    EXPECT_TRUE(callback_called);
}

TEST(NoTLSFuture, void_then_calls_back_immediately_when_promise_is_already_fulfilled)
{
    mcl::NoTLSPromise<void> promise;
    promise.set_value();

    auto future = promise.get_future();
    bool callback_called{false};

    future.then([&callback_called](auto&&) { callback_called = true;});
    EXPECT_TRUE(callback_called);
}

TEST(NoTLSFuture, then_calls_back_with_correct_value_when_promise_is_already_fulfilled)
{
    constexpr int expected{0xfeed};

    mcl::NoTLSPromise<int> promise;
    promise.set_value(expected);

    auto future = promise.get_future();
    future.then(
        [expected](auto&& result)
        {
            using namespace testing;

            auto value = result.get();
            EXPECT_THAT(value, Eq(expected));
        });
}

TEST(NoTLSFuture, then_is_not_called_until_promise_is_fulfilled_by_copy)
{
    constexpr char const* expected{"And then nothing turned itself inside out"};

    mcl::NoTLSPromise<std::string> promise;
    auto future = promise.get_future();

    bool called{false};
    auto result = future.then(
        [&called, expected](auto&& value)
        {
            using namespace testing;
            called = true;
            auto result = value.get();
            EXPECT_THAT(result, StrEq(expected));
        });

    EXPECT_FALSE(called);

    std::string expected_copy{expected};

    promise.set_value(expected_copy);
    EXPECT_TRUE(called);
}

TEST(NoTLSFuture, then_is_not_called_until_promise_is_fulfilled_by_moving)
{
    constexpr char const* expected{"And then nothing turned itself inside out"};

    mcl::NoTLSPromise<std::string> promise;
    auto future = promise.get_future();

    bool called{false};
    auto result = future.then(
        [&called, expected](auto&& value)
        {
            using namespace testing;
            called = true;
            auto result = value.get();
            EXPECT_THAT(result, StrEq(expected));
        });

    EXPECT_FALSE(called);

    promise.set_value(std::string{expected});
    EXPECT_TRUE(called);
}

TEST(NoTLSFuture, void_then_is_not_called_until_promise_is_fulfilled)
{
    mcl::NoTLSPromise<void> promise;
    auto future = promise.get_future();

    bool called{false};
    auto far_future = future.then(
        [&called](auto&& result)
        {
            result.get();
            called = true;
        });

    EXPECT_FALSE(called);

    promise.set_value();
    EXPECT_TRUE(called);
}

TEST(NoTLSFuture, then_is_called_when_promise_is_broken)
{
    auto promise = std::make_unique<mcl::NoTLSPromise<int>>();
    auto future = promise->get_future();

    bool called{false};
    auto far_future = future.then(
        [&called](auto&& result)
        {
            EXPECT_THROW(result.get(), std::runtime_error);
            called = true;
        });

    EXPECT_FALSE(called);
    promise.reset();
    EXPECT_TRUE(called);
}

TEST(NoTLSFuture, then_is_called_when_void_promise_is_broken)
{
    auto promise = std::make_unique<mcl::NoTLSPromise<void>>();
    auto future = promise->get_future();

    bool called{false};
    auto far_future = future.then(
        [&called](auto&& result)
        {
            EXPECT_THROW(result.get(), std::runtime_error);
            called = true;
        });

    EXPECT_FALSE(called);
    promise.reset();
    EXPECT_TRUE(called);
}

TEST(NoTLSFuture, destruction_of_future_blocks_if_unsatisfied)
{
    mcl::NoTLSPromise<float> promise;

    mt::Signal done;

    mt::AutoJoinThread a{
        [](mt::Signal& unblocked, mcl::NoTLSFuture<float>&& future)
        {
            {
                auto killer = std::move(future);
            }
            unblocked.raise();
        },
        std::ref(done),
        promise.get_future()};

    EXPECT_FALSE(done.wait_for(std::chrono::seconds{1}));

    promise.set_value(3.1415f);

    EXPECT_TRUE(done.wait_for(std::chrono::seconds{1}));
}

TEST(NoTLSFuture, promises_are_write_once)
{
    mcl::NoTLSPromise<float> non_void_promise;
    mcl::NoTLSPromise<void> void_promise;

    non_void_promise.set_value(3.5f);
    void_promise.set_value();

    EXPECT_THROW(non_void_promise.set_value(2.0f), std::runtime_error);
    EXPECT_THROW(void_promise.set_value(), std::runtime_error);
}

TEST(NoTLSFuture, setting_continuation_invalidates_future)
{
    auto non_void_promise = std::make_unique<mcl::NoTLSPromise<int*>>();
    auto void_promise = std::make_unique<mcl::NoTLSPromise<void>>();

    auto non_void_future = non_void_promise->get_future();
    auto void_future = void_promise->get_future();

    auto far_non_void_future = non_void_future.then([](auto&&) {});
    auto far_void_future = void_future.then([](auto&&) {});

    EXPECT_TRUE(far_non_void_future.valid());
    EXPECT_TRUE(far_void_future.valid());
    EXPECT_FALSE(non_void_future.valid());
    EXPECT_FALSE(void_future.valid());

    // And break our promises so ~NoTLSFuture is unblocked
    non_void_promise.reset();
    void_promise.reset();
}

TEST(NoTLSFuture, retrieving_future_twice_throws)
{
    auto non_void_promise = std::make_unique<mcl::NoTLSPromise<int*>>();
    auto void_promise = std::make_unique<mcl::NoTLSPromise<void>>();

    auto non_void_future = non_void_promise->get_future();
    auto void_future = void_promise->get_future();

    EXPECT_THROW(non_void_promise->get_future(), std::runtime_error);
    EXPECT_THROW(void_promise->get_future(), std::runtime_error);

    void_promise.reset();
    non_void_promise.reset();
}

TEST(NoTLSFuture, future_returned_from_then_resolves_to_return_value_of_closure)
{
    using namespace testing;
    mcl::NoTLSPromise<int> base_promise;

    auto base_future = base_promise.get_future();

    mcl::NoTLSFuture<std::string> transformed_future = base_future.then(
        [](auto&& resolved_future)
        {
            return std::to_string(resolved_future.get());
        });

    base_promise.set_value(254);
    EXPECT_THAT(transformed_future.get(), StrEq("254"));
}

TEST(NoTLSFuture, can_transport_move_only_items)
{
    using namespace testing;
    mcl::NoTLSPromise<std::unique_ptr<int>> promise;

    auto future = promise.get_future();

    promise.set_value(std::make_unique<int>(42));

    EXPECT_THAT(*future.get(), Eq(42));
}

TEST(NoTLSFuture, future_is_invalid_after_retreival)
{
    mcl::NoTLSPromise<int> promise;

    auto future = promise.get_future();

    promise.set_value(42);
    future.get();

    EXPECT_FALSE(future.valid());
}

TEST(NoTLSFuture, void_future_is_invalid_after_retreival)
{
    mcl::NoTLSPromise<void> promise;

    auto future = promise.get_future();

    promise.set_value();
    future.get();

    EXPECT_FALSE(future.valid());
}

TEST(NoTLSFuture, exception_in_continuation_is_captured)
{
    using namespace testing;
    mcl::NoTLSPromise<int> promise;

    auto future = promise.get_future();

    auto transformed_future = future.then(
        [](auto&&)
        {
            BOOST_THROW_EXCEPTION(std::system_error(ENODATA, std::system_category(), "Theyms takin my data"));
        });

    promise.set_value(42);

    try
    {
        transformed_future.get();
    }
    catch (std::system_error const& err)
    {
        EXPECT_THAT(err.code(), Eq(std::error_code(ENODATA, std::system_category())));
        EXPECT_THAT(err.what(), HasSubstr("Theyms takin my data"));
    }
}

TEST(NoTLSFuture, exception_in_void_continuation_is_captured)
{
    using namespace testing;
    mcl::NoTLSPromise<void> promise;

    auto future = promise.get_future();

    auto transformed_future = future.then(
        [](auto&&)
        {
            BOOST_THROW_EXCEPTION(std::system_error(ENODATA, std::system_category(), "Theyms takin my data"));
        });

    promise.set_value();

    try
    {
        transformed_future.get();
    }
    catch (std::system_error const& err)
    {
        EXPECT_THAT(err.code(), Eq(std::error_code(ENODATA, std::system_category())));
        EXPECT_THAT(err.what(), HasSubstr("Theyms takin my data"));
    }
}

TEST(NoTLSFuture, detach_allows_destruction_before_readiness)
{
    using namespace testing;
    mcl::NoTLSPromise<void> promise;

    bool continuation_called{false};

    {
        auto future = promise.get_future();

        future.then([&continuation_called](auto&&) { continuation_called = true; }).detach();
    }

    promise.set_value();

    EXPECT_TRUE(continuation_called);
}

TEST(NoTLSFuture, detached_future_still_runs_continuation)
{
    using namespace testing;
    mcl::NoTLSPromise<std::string> promise;

    constexpr char const* value = "Hello my pretties!";

    bool continuation_called{false};

    {
        auto future = promise.get_future();

        future.then(
            [&continuation_called, value](auto&& resolved_future)
            {
                EXPECT_THAT(resolved_future.get(), StrEq(value));
                continuation_called = true;
            }).detach();
    }

    promise.set_value(value);

    EXPECT_TRUE(continuation_called);
}

TEST(NoTLSFuture, detached_future_still_runs_continuation_on_broken_promise)
{
    using namespace testing;

    bool continuation_called{false};
    {
        mcl::NoTLSPromise<void> promise;

        {
            auto future = promise.get_future();

            future.then(
                [&continuation_called](auto&& resolved_future)
                {
                    continuation_called = true;
                    try
                    {
                        resolved_future.get();
                        ADD_FAILURE() << "Unexpectedly failed to raise an exception";
                    }
                    catch (std::exception const& e)
                    {
                        EXPECT_THAT(e.what(), HasSubstr("broken_promise"));
                    }
                }).detach();
        }
    }

    EXPECT_TRUE(continuation_called);
}
