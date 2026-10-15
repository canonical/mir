/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/logging/tag.h>
#include <mir/options/program_option.h>
#include <boost/program_options/detail/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <mir/logging/logger.h>
#include <mir/log.h>
#include "src/core/logging/log_deduplicator.h"

#include <boost/program_options.hpp>
#include <chrono>
#include <format>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <cstring>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ml = mir::logging;

class MockLogger : public mir::logging::Logger
{
public:
    MOCK_METHOD(void, log, (mir::logging::Severity severity, std::string const&message, std::string const& component), (override));
};

class TestLog : public testing::Test
{
public:
    void SetUp() override
    {
        logger = std::make_shared<testing::NiceMock<MockLogger>>();
        mir::logging::set_logger(logger);
    }

    void TearDown() override
    {
        // Replace Mir's logger so that we own the last reference to our MockLogger
        // allowing it to be destroyed and verified after the test finishes.
        mir::logging::set_logger(std::make_shared<MockLogger>());
        ASSERT_TRUE(logger.unique()) << "Test needs to destroy the MockLogger to verify assertions";
        logger.reset();
    }

    std::shared_ptr<testing::NiceMock<MockLogger>> logger;
};

TEST_F(TestLog, log_calls_logger)
{
    auto const severity = mir::logging::Severity::informational;
    auto const message = "test message";
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(severity, message, std::string{ml::tag::name(tag)}));

    ml::tag::set_severity(ml::tag::name(tag), severity);
    mir::log(severity, {tag}, message);
}

TEST_F(TestLog, log_calls_logger_with_colon_separated_component_for_multiple_tags)
{
    auto const severity = mir::logging::Severity::debug;
    auto const message = "A message, and a system of messages";
    std::string expected_component = "graphics:wayland";

    EXPECT_CALL(*logger, log(severity, message, expected_component));

    ml::tag::set_severity("graphics", severity);
    mir::log(severity, {ml::graphics(), ml::wayland()}, message);
}

TEST_F(TestLog, log_debug_works)
{
    constexpr auto fmt_string = "{} around the world";
    auto const value = "Twice";
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(ml::Severity::debug, message + "!", std::string{ml::tag::name(tag)})).Times(1);
    EXPECT_CALL(*logger, log(ml::Severity::debug, message, std::string{ml::tag::name(tag)})).Times(1);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::debug);
    mir::log_debug({tag}, message + "!");
    mir::log_debug({tag}, fmt_string, value);
}

TEST_F(TestLog, log_info_works)
{
    constexpr auto fmt_string = "Arbitrarium {}";
    auto const value = 18;
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(ml::Severity::informational, message + "!", std::string{ml::tag::name(tag)})).Times(1);
    EXPECT_CALL(*logger, log(ml::Severity::informational, message, std::string{ml::tag::name(tag)})).Times(1);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::informational);
    mir::log_info({tag}, message + "!");
    mir::log_info({tag}, fmt_string, value);
}

TEST_F(TestLog, log_warning_works)
{
    constexpr auto fmt_string = "Abort! Abort! {:.2f}";
    auto const value = 23.22222222;
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(ml::Severity::warning, message + "!", std::string{ml::tag::name(tag)})).Times(1);
    EXPECT_CALL(*logger, log(ml::Severity::warning, message, std::string{ml::tag::name(tag)})).Times(1);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::warning);
    mir::log_warning({tag}, message + "!");
    mir::log_warning({tag}, fmt_string, value);
}

TEST_F(TestLog, log_error_works)
{
    constexpr std::string_view fmt_string = "Terrain warning: {}m";
    auto const value = 100;
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(ml::Severity::error, message + "!", std::string{ml::tag::name(tag)})).Times(1);
    EXPECT_CALL(*logger, log(ml::Severity::error, message, std::string{ml::tag::name(tag)})).Times(1);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::error);
    mir::log_error({tag}, message + "!");
    mir::log_error({tag}, fmt_string, value);
}

TEST_F(TestLog, log_critical_works)
{
    constexpr std::string_view fmt_string = "Critical error: {}";
    auto const value = "ENOCAKE";
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(ml::Severity::critical, message + "!", std::string{ml::tag::name(tag)})).Times(1);
    EXPECT_CALL(*logger, log(ml::Severity::critical, message, std::string{ml::tag::name(tag)})).Times(1);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::critical);
    mir::log_critical({tag}, message + "!");
    mir::log_critical({tag}, fmt_string, value);
}

TEST_F(TestLog, can_use_format_string)
{
    auto const severity = mir::logging::Severity::informational;
    auto const& tag = ml::input();
    constexpr std::string_view fmt_string = "now: {}, then: {:10.5f}, again: {}";

    auto const now = std::chrono::system_clock::now();
    auto const a_float = 3.1415f;
    auto const a_stringish = "Merrily, merrily, merrily, merrily";

    auto const message = std::format(fmt_string, now, a_float, a_stringish);

    EXPECT_CALL(*logger, log(severity, message, std::string{ml::tag::name(tag)}));

    ml::tag::set_severity(ml::tag::name(tag), severity);
    mir::log(severity, {tag}, message);
}

TEST_F(TestLog, can_set_severity_by_full_path)
{
    auto const severity = mir::logging::Severity::informational;

    auto const &tag_a = ml::create_tag(ml::base(), "a");
    auto const &tag_b = ml::create_tag(tag_a, "b");

    ASSERT_FALSE(ml::logging_enabled_for(tag_b, severity)) << "Test would spuriously pass";

    ml::tag::set_severity(std::format("{}/{}/{}", ml::tag::name(ml::base()), ml::tag::name(tag_a), ml::tag::name(tag_b)), severity);

    EXPECT_TRUE(ml::logging_enabled_for(tag_b, severity));
}

TEST_F(TestLog, attempt_to_set_severity_by_subpath_fails)
{
    auto const severity = mir::logging::Severity::debug;

    auto const &tag_a = ml::create_tag(ml::base(), "a");
    auto const &tag_b = ml::create_tag(tag_a, "b");

    ASSERT_FALSE(ml::logging_enabled_for(tag_b, severity)) << "Test would spuriously pass";

    EXPECT_THROW(
        ml::tag::set_severity(std::format("{}/{}", ml::tag::name(tag_a), ml::tag::name(tag_b)), severity);,
        std::out_of_range
    );
}

TEST_F(TestLog, can_set_severity_by_full_path_when_tag_name_is_the_same)
{
    auto const severity = mir::logging::Severity::informational;

    auto const &parent_a = ml::create_tag(ml::graphics(), "atomic-kms");
    auto const &parent_b = ml::create_tag(ml::graphics(), "gbm-kms");

    ml::create_tag(parent_a, "bypass");
    auto const& bypass_b = ml::create_tag(parent_b, "bypass");

    ASSERT_FALSE(ml::logging_enabled_for(bypass_b, severity)) << "Test would spuriously pass";

    ml::tag::set_severity(std::format("base/graphics/gbm-kms/bypass"), severity);

    EXPECT_TRUE(ml::logging_enabled_for(bypass_b, severity));
}

TEST_F(TestLog, repeated_message_is_logged_once_within_window)
{
    auto const severity = ml::Severity::warning;
    auto const message = std::string{"presentation failed"};

    EXPECT_CALL(*logger, log(severity, message, std::string{"gbm-kms"})).Times(1);

    mir::log(severity, "gbm-kms", message);
    mir::log(severity, "gbm-kms", "presentation %s", "failed");
}

TEST_F(TestLog, set_logger_resets_deduplication_state)
{
    auto const severity = ml::Severity::warning;
    auto const message = std::string{"presentation failed"};
    auto const replacement = std::make_shared<testing::NiceMock<MockLogger>>();

    EXPECT_CALL(*logger, log(severity, message, std::string{"gbm-kms"})).Times(1);
    EXPECT_CALL(*replacement, log(severity, message, std::string{"gbm-kms"})).Times(1);

    mir::log(severity, "gbm-kms", message);
    ml::set_logger(replacement);
    mir::log(severity, "gbm-kms", message);
    mir::log(severity, "gbm-kms", message);
}

TEST_F(TestLog, reinstalling_same_logger_resets_deduplication_state)
{
    auto const severity = ml::Severity::warning;
    auto const message = std::string{"presentation failed"};

    EXPECT_CALL(*logger, log(severity, message, std::string{"gbm-kms"})).Times(2);

    mir::log(severity, "gbm-kms", message);
    mir::log(severity, "gbm-kms", message);
    ml::set_logger(logger);
    mir::log(severity, "gbm-kms", message);
    mir::log(severity, "gbm-kms", message);
}

TEST_F(TestLog, disabled_tagged_calls_do_not_consume_deduplication_history)
{
    auto const severity = ml::Severity::debug;
    auto const message = "tagged failure";
    auto const& tag = ml::graphics();

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::error);
    EXPECT_CALL(*logger, log(testing::_, testing::_, testing::_)).Times(0);
    mir::log(severity, {tag}, message);
    testing::Mock::VerifyAndClearExpectations(logger.get());

    ml::tag::set_severity(ml::tag::name(tag), severity);
    EXPECT_CALL(*logger, log(severity, std::string{message}, std::string{"graphics"})).Times(1);
    mir::log(severity, {tag}, message);
    mir::log(severity, {tag}, message);
}

class LogDeduplicatorTest : public testing::Test
{
public:
    std::optional<std::string> filter(std::string const& message)
    {
        return deduplicator.filter(ml::Severity::warning, "gbm-kms", message);
    }

    void advance(std::chrono::seconds by)
    {
        now += by;
    }

    std::chrono::steady_clock::time_point now{};
    ml::detail::LogDeduplicator deduplicator{std::chrono::seconds{5}, [this] { return now; }};
};


TEST_F(LogDeduplicatorTest, reprint_after_window_has_count_of_suppressed_plus_one)
{
    EXPECT_EQ(filter("presentation failed"), "presentation failed");
    for (int i = 0; i < 4; ++i)
    {
        advance(std::chrono::seconds{1});
        EXPECT_EQ(filter("presentation failed"), std::nullopt);
    }

    advance(std::chrono::seconds{1});
    EXPECT_EQ(filter("presentation failed"), "presentation failed (x5)");
    EXPECT_EQ(filter("presentation failed"), std::nullopt);
    advance(std::chrono::seconds{5});
    EXPECT_EQ(filter("presentation failed"), "presentation failed (x2)");
}

TEST_F(LogDeduplicatorTest, reprint_has_no_suffix_when_nothing_was_suppressed)
{
    filter("presentation failed");
    advance(std::chrono::seconds{5});

    EXPECT_EQ(filter("presentation failed"), "presentation failed");
}


TEST_F(LogDeduplicatorTest, different_severity_component_or_message_are_not_merged)
{
    EXPECT_EQ(deduplicator.filter(ml::Severity::warning, "gbm-kms", "presentation failed"), "presentation failed");
    EXPECT_EQ(deduplicator.filter(ml::Severity::error, "gbm-kms", "presentation failed"), "presentation failed");
    EXPECT_EQ(deduplicator.filter(ml::Severity::warning, "atomic-kms", "presentation failed"), "presentation failed");
    EXPECT_EQ(deduplicator.filter(ml::Severity::warning, "gbm-kms", "presentation recovered"), "presentation recovered");
}

class PendingRetentionTest : public LogDeduplicatorTest, public testing::WithParamInterface<std::chrono::seconds>
{
};

TEST_P(PendingRetentionTest, prunes_singletons_but_keeps_pending_counts)
{
    filter("kept");
    filter("dropped");
    filter("also dropped");
    ASSERT_EQ(deduplicator.size(), 3u);
    advance(std::chrono::seconds{1});
    EXPECT_EQ(filter("kept"), std::nullopt);

    advance(GetParam() - std::chrono::seconds{1});
    filter("unrelated");
    EXPECT_EQ(deduplicator.size(), 2u);
    EXPECT_EQ(filter("kept"), "kept (x2)");
}

INSTANTIATE_TEST_SUITE_P(WindowAndRetentionBoundaries, PendingRetentionTest,
    testing::Values(std::chrono::seconds{5}));
