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

#include <boost/program_options.hpp>
#include <chrono>
#include <format>
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

    EXPECT_CALL(*logger, log(ml::Severity::debug, message, std::string{ml::tag::name(tag)})).Times(2);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::debug);
    mir::log_debug({tag}, message);
    mir::log_debug({tag}, fmt_string, value);
}

TEST_F(TestLog, log_info_works)
{
    constexpr auto fmt_string = "Arbitrarium {}";
    auto const value = 18;
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(ml::Severity::informational, message, std::string{ml::tag::name(tag)})).Times(2);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::informational);
    mir::log_info({tag}, message);
    mir::log_info({tag}, fmt_string, value);
}

TEST_F(TestLog, log_warning_works)
{
    constexpr auto fmt_string = "Abort! Abort! {:.2f}";
    auto const value = 23.22222222;
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(ml::Severity::warning, message, std::string{ml::tag::name(tag)})).Times(2);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::warning);
    mir::log_warning({tag}, message);
    mir::log_warning({tag}, fmt_string, value);
}

TEST_F(TestLog, log_error_works)
{
    constexpr std::string_view fmt_string = "Terrain warning: {}m";
    auto const value = 100;
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(ml::Severity::error, message, std::string{ml::tag::name(tag)})).Times(2);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::error);
    mir::log_error({tag}, message);
    mir::log_error({tag}, fmt_string, value);
}

TEST_F(TestLog, log_critical_works)
{
    constexpr std::string_view fmt_string = "Critical error: {}";
    auto const value = "ENOCAKE";
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(ml::Severity::critical, message, std::string{ml::tag::name(tag)})).Times(2);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::critical);
    mir::log_critical({tag}, message);
    mir::log_critical({tag}, fmt_string, value);
}

TEST_F(TestLog, can_use_format_string)
{
    auto const severity = mir::logging::Severity::informational;
    auto const& tag = ml::input();
    constexpr std::string_view fmt_string = "now: {}, then: {:10.5f}, again: {:.{}}";

    auto const now = std::chrono::system_clock::now();
    auto const a_float = 3.1415f;
    auto const a_stringish = "Merrily, merrily, merrily, merrily";

    auto const message = std::format(fmt_string, now, a_float, a_stringish, std::strlen("Merrily"));

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
