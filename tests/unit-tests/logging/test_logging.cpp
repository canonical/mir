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

#include <exception>
#include <mir/logging/tag.h>
#include <mir/options/program_option.h>
#include <boost/program_options/detail/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <mir/logging/logger.h>

#define MIR_LOG_COMPONENT "tests"
#include <mir/log.h>

#include <boost/program_options.hpp>
#include <chrono>
#include <format>
#include <stdexcept>
#include <string_view>
#include <cstring>

#include <mir/test/doubles/mock_logger.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ml = mir::logging;
namespace mtd = mir::test::doubles;
using namespace testing;

class TestLog : public testing::Test
{
public:
    void SetUp() override
    {
        logger = std::make_shared<testing::NiceMock<mtd::MockLogger>>();
        mir::logging::set_logger(logger);
    }

    void TearDown() override
    {
        // Replace Mir's logger so that we own the last reference to our MockLogger
        // allowing it to be destroyed and verified after the test finishes.
        mir::logging::set_logger(std::make_shared<mtd::MockLogger>());
        ASSERT_TRUE(logger.unique()) << "Test needs to destroy the MockLogger to verify assertions";
        logger.reset();
    }

    std::shared_ptr<testing::NiceMock<mtd::MockLogger>> logger;
};

TEST_F(TestLog, log_calls_logger)
{
    using mtd::LogMatching;
    using mtd::IsTag;
    auto const severity = mir::logging::Severity::informational;
    auto const message = "test message";
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(LogMatching(Eq(severity), Eq(std::string{message}), ElementsAre(IsTag(tag)))));

    ml::tag::set_severity(ml::tag::name(tag), severity);
    mir::log(severity, {tag}, message);
}

TEST_F(TestLog, log_calls_logger_with_colon_separated_component_for_multiple_tags)
{
    using mtd::LogMatching;
    using mtd::IsTag;
    auto const severity = mir::logging::Severity::debug;
    auto const message = "A message, and a system of messages";

    EXPECT_CALL(*logger, log(LogMatching(Eq(severity), Eq(std::string{message}), ElementsAre(IsTag(ml::graphics()), IsTag(ml::wayland())))));

    ml::tag::set_severity("graphics", severity);
    mir::log(severity, {ml::graphics(), ml::wayland()}, message);
}

TEST_F(TestLog, log_debug_works)
{
    using mtd::LogMatching;
    using mtd::IsTag;
    constexpr auto fmt_string = "{} around the world";
    auto const value = "Twice";
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(LogMatching(Eq(ml::Severity::debug), Eq(message), ElementsAre(IsTag(tag))))).Times(2);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::debug);
    mir::log_debug({tag}, message);
    mir::log_debug({tag}, fmt_string, value);
}

TEST_F(TestLog, log_info_works)
{
    using mtd::LogMatching;
    using mtd::IsTag;
    constexpr auto fmt_string = "Arbitrarium {}";
    auto const value = 18;
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(LogMatching(Eq(ml::Severity::informational), Eq(message), ElementsAre(IsTag(tag))))).Times(2);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::informational);
    mir::log_info({tag}, message);
    mir::log_info({tag}, fmt_string, value);
}

TEST_F(TestLog, log_warning_works)
{
    using mtd::LogMatching;
    using mtd::IsTag;
    constexpr auto fmt_string = "Abort! Abort! {:.2f}";
    auto const value = 23.22222222;
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(LogMatching(Eq(ml::Severity::warning), Eq(message), ElementsAre(IsTag(tag))))).Times(2);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::warning);
    mir::log_warning({tag}, message);
    mir::log_warning({tag}, fmt_string, value);
}

TEST_F(TestLog, log_error_works)
{
    using mtd::LogMatching;
    using mtd::IsTag;
    constexpr std::string_view fmt_string = "Terrain warning: {}m";
    auto const value = 100;
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(LogMatching(Eq(ml::Severity::error), Eq(message), ElementsAre(IsTag(tag))))).Times(2);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::error);
    mir::log_error({tag}, message);
    mir::log_error({tag}, fmt_string, value);
}

TEST_F(TestLog, log_critical_works)
{
    using mtd::LogMatching;
    using mtd::IsTag;
    constexpr std::string_view fmt_string = "Critical error: {}";
    auto const value = "ENOCAKE";
    auto const message = std::format(fmt_string, value);
    auto const& tag = ml::base();

    EXPECT_CALL(*logger, log(LogMatching(Eq(ml::Severity::critical), Eq(message), ElementsAre(IsTag(tag))))).Times(2);

    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::critical);
    mir::log_critical({tag}, message);
    mir::log_critical({tag}, fmt_string, value);
}

TEST_F(TestLog, can_use_format_string)
{
    using mtd::LogMatching;
    using mtd::IsTag;
    auto const severity = mir::logging::Severity::informational;
    auto const& tag = ml::input();
    constexpr std::string_view fmt_string = "now: {}, then: {:10.5f}, again: {}";

    auto const now = std::chrono::system_clock::now();
    auto const a_float = 3.1415f;
    auto const a_stringish = "Merrily, merrily, merrily, merrily";

    auto const message = std::format(fmt_string, now, a_float, a_stringish);

    EXPECT_CALL(*logger, log(LogMatching(Eq(severity), Eq(message), ElementsAre(IsTag(tag)))));

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

TEST_F(TestLog, logging_captures_source_location)
{
    auto const& tag = ml::base();

    // Ensure that all log messages are propagated
    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::debug);

    std::source_location logged_loc, next_line;

    ON_CALL(*logger, log(_))
        .WillByDefault(
            Invoke(
                [&logged_loc](ml::Event const& ev)
                {
                    logged_loc = ev.location();
                }));

    mir::log(ml::Severity::informational, {tag}, "Hello");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log(ml::Severity::informational, {tag}, "I take a {} string", "format");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log(ml::Severity::debug, MIR_LOG_COMPONENT, "And the %s works, too", "printf API");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    try
    {
        throw std::runtime_error{"Just to test"};
    }
    catch (std::exception const&)
    {
        auto prev_line = std::source_location::current();
        mir::log(
            ml::Severity::debug,
            MIR_LOG_COMPONENT,
            std::current_exception(),
            "The exception_ptr API works");

        EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
        EXPECT_THAT(logged_loc.line(), Eq(prev_line.line() + 1));
    }

    try
    {
        throw "Don't do this, but we need to check the non-std::exception path";
    }
    catch (...)
    {
        auto prev_line = std::source_location::current();
        mir::log(
            ml::Severity::debug,
            MIR_LOG_COMPONENT,
            std::current_exception(),
            "The exception_ptr API works");

        EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
        EXPECT_THAT(logged_loc.line(), Eq(prev_line.line() + 1));
    }

    mir::log(ml::Severity::debug, MIR_LOG_COMPONENT, std::string{"The string API works"});
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));
}

TEST_F(TestLog, log_debug_captures_source_location)
{
    auto const& tag = ml::base();

    // Ensure that all log messages are propagated
    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::debug);

    std::source_location logged_loc, next_line;

    ON_CALL(*logger, log(_))
        .WillByDefault(
            Invoke(
                [&logged_loc](ml::Event const& ev)
                {
                    logged_loc = ev.location();
                }));

    mir::log_debug({tag}, "Hello");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log_debug({tag}, "I take a {} string", "format");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log_debug("And the %s works, too", "printf API");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));
}

TEST_F(TestLog, log_info_captures_source_location)
{
    auto const& tag = ml::base();

    // Ensure that all log messages are propagated
    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::debug);

    std::source_location logged_loc, next_line;

    ON_CALL(*logger, log(_))
        .WillByDefault(
            Invoke(
                [&logged_loc](ml::Event const& ev)
                {
                    logged_loc = ev.location();
                }));

    mir::log_info({tag}, "Hello");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log_info({tag}, "I take a {} string", "format");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log_info("And the %s works, too", "printf API");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log_info(std::string{"The string API works"});
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));
}

TEST_F(TestLog, log_warning_captures_source_location)
{
    auto const& tag = ml::base();

    // Ensure that all log messages are propagated
    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::debug);

    std::source_location logged_loc, next_line;

    ON_CALL(*logger, log(_))
        .WillByDefault(
            Invoke(
                [&logged_loc](ml::Event const& ev)
                {
                    logged_loc = ev.location();
                }));

    mir::log_warning({tag}, "Hello");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log_warning({tag}, "I take a {} string", "format");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log_warning("And the %s works, too", "printf API");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));
}

TEST_F(TestLog, log_error_captures_source_location)
{
    auto const& tag = ml::base();

    // Ensure that all log messages are propagated
    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::debug);

    std::source_location logged_loc, next_line;

    ON_CALL(*logger, log(_))
        .WillByDefault(
            Invoke(
                [&logged_loc](ml::Event const& ev)
                {
                    logged_loc = ev.location();
                }));

    mir::log_error({tag}, "Hello");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log_error({tag}, "I take a {} string", "format");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log_error("And the %s works, too", "printf API");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));
}

TEST_F(TestLog, log_critical_captures_source_location)
{
    auto const& tag = ml::base();

    // Ensure that all log messages are propagated
    ml::tag::set_severity(ml::tag::name(tag), ml::Severity::debug);

    std::source_location logged_loc, next_line;

    ON_CALL(*logger, log(_))
        .WillByDefault(
            Invoke(
                [&logged_loc](ml::Event const& ev)
                {
                    logged_loc = ev.location();
                }));

    mir::log_critical({tag}, "Hello");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log_critical({tag}, "I take a {} string", "format");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));

    mir::log_critical("And the %s works, too", "printf API");
    next_line = std::source_location::current();

    EXPECT_THAT(logged_loc.file_name(), StrEq(next_line.file_name()));
    EXPECT_THAT(logged_loc.line(), Eq(next_line.line() - 1));
}
