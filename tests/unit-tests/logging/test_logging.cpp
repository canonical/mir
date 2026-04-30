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

#include "mir/options/program_option.h"
#include <boost/program_options/detail/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <mir/logging/logger.h>
#include <mir/log.h>

#include <boost/program_options.hpp>
#include <chrono>
#include <format>
#include <string_view>
#include <cstring>
#include <list>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ml = mir::logging;

class CmdLine
{
public:
    CmdLine()
    {
        add_argument("mir_unit_tests");
    }

    void add_argument(std::string_view arg)
    {
        arg_storage.emplace_back(arg);
        args.push_back(arg_storage.back().c_str());
    }

    /* This *should* be argv() const -> char const* const*, but
     * mo::ProgramOptions::parse_arguments() incorrectly takes a char const**
     */
    auto argv() -> char const**
    {
        return args.data();
    }

    auto argc() const -> int
    {
        return args.size();
    }

private:
    std::vector<char const*> args;
    std::list<std::string> arg_storage;
};

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

    void set_log_level(std::string_view tag_name, ml::Severity sev)
    {
        /*
         * We don't have access to the Tag internals, but we *can* bounce
         * through the options infrastructure.
         */
        boost::program_options::options_description desc;
        ml::add_logging_options(desc.add_options());

        CmdLine cmdline{};
        cmdline.add_argument(std::format("--log-level={}={}", tag_name, sev));

        mir::options::ProgramOption mo;
        mo.parse_arguments(desc, cmdline.argc(), cmdline.argv());
    }

    std::shared_ptr<testing::NiceMock<MockLogger>> logger;
};

TEST_F(TestLog, log_calls_logger)
{
    auto const severity = mir::logging::Severity::informational;
    auto const message = "test message";

    EXPECT_CALL(*logger, log(severity, message, "base"));

    set_log_level("base", severity);
    mir::log(severity, {ml::base()}, message);
}

TEST_F(TestLog, log_calls_logger_with_colon_separated_component_for_multiple_tags)
{
    auto const severity = mir::logging::Severity::debug;
    auto const message = "A message, and a system of messages";
    std::string expected_component = "graphics:wayland";


    EXPECT_CALL(*logger, log(severity, message, expected_component));

    set_log_level("graphics", severity);
    mir::log(severity, {ml::graphics(), ml::wayland()}, message);
}

TEST_F(TestLog, log_debug_works)
{
    constexpr auto fmt_string = "{} around the world";
    auto const value = "Twice";
    auto const message = std::format(fmt_string, value);

    EXPECT_CALL(*logger, log(ml::Severity::debug, message, "base")).Times(2);

    set_log_level("base", ml::Severity::debug);
    mir::log_debug({ml::base()}, message);
    mir::log_debug({ml::base()}, fmt_string, value);
}

TEST_F(TestLog, log_info_works)
{
    constexpr auto fmt_string = "Arbitrarium {}";
    auto const value = 18;
    auto const message = std::format(fmt_string, value);

    EXPECT_CALL(*logger, log(ml::Severity::informational, message, "base")).Times(2);

    set_log_level("base", ml::Severity::informational);
    mir::log_info({ml::base()}, message);
    mir::log_info({ml::base()}, fmt_string, value);
}

TEST_F(TestLog, log_warning_works)
{
    constexpr auto fmt_string = "Abort! Abort! {:.2f}";
    auto const value = 23.22222222;
    auto const message = std::format(fmt_string, value);

    EXPECT_CALL(*logger, log(ml::Severity::warning, message, "base")).Times(2);

    set_log_level("base", ml::Severity::warning);
    mir::log_warning({ml::base()}, message);
    mir::log_warning({ml::base()}, fmt_string, value);
}

TEST_F(TestLog, log_error_works)
{
    constexpr std::string_view fmt_string = "Terrain warning: {}m";
    auto const value = 100;
    auto const message = std::format(fmt_string, value);

    EXPECT_CALL(*logger, log(ml::Severity::error, message, "base")).Times(2);

    set_log_level("base", ml::Severity::error);
    mir::log_error({ml::base()}, message);
    mir::log_error({ml::base()}, fmt_string, value);
}

TEST_F(TestLog, log_critical_works)
{
    constexpr std::string_view fmt_string = "Critical error: {}";
    auto const value = "ENOCAKE";
    auto const message = std::format(fmt_string, value);

    EXPECT_CALL(*logger, log(ml::Severity::critical, message, "base")).Times(2);

    set_log_level("base", ml::Severity::critical);
    mir::log_critical({ml::base()}, message);
    mir::log_critical({ml::base()}, fmt_string, value);
}

TEST_F(TestLog, can_use_format_string)
{
    auto const severity = mir::logging::Severity::informational;
    constexpr std::string_view fmt_string = "now: {}, then: {:10.5f}, again: {:.{}}";

    auto const now = std::chrono::system_clock::now();
    auto const a_float = 3.1415f;
    auto const a_stringish = "Merrily, merrily, merrily, merrily";

    auto const message = std::format(fmt_string, now, a_float, a_stringish, std::strlen("Merrily"));

    EXPECT_CALL(*logger, log(severity, message, "input"));

    set_log_level("input", severity);
    mir::log(severity, {ml::input()}, message);
}

TEST_F(TestLog, can_set_options)
{
    namespace po = boost::program_options;
    using namespace testing;

    po::options_description desc;
    ml::add_logging_options(desc.add_options());

    CmdLine cmdline{};
    cmdline.add_argument("--log-level=base=debug");
    cmdline.add_argument("--log-level=wayland:critical");

    mir::options::ProgramOption mo;
    mo.parse_arguments(desc, cmdline.argc(), cmdline.argv());

    EXPECT_THAT(mo.is_set("log-level"), Eq(true));
}

TEST_F(TestLog, invalid_log_levels_are_ignored)
{
    namespace po = boost::program_options;
    using namespace testing;

    po::options_description desc;
    ml::add_logging_options(desc.add_options());

    CmdLine cmdline{};
    // Log-level is of the form “tag=severity”
    cmdline.add_argument("--log-level=base");
    // Trying to set the log level of an unknown tag
    cmdline.add_argument("--log-level=not_a_tag=info");
    // Trying to set the log level to an invalid severity
    cmdline.add_argument("--log-level=input=not_a_severity");

    mir::options::ProgramOption mo;

    EXPECT_NO_THROW(mo.parse_arguments(desc, cmdline.argc(), cmdline.argv()));
}

TEST_F(TestLog, can_set_multiple_different_log_levels)
{
    namespace po = boost::program_options;
    using namespace testing;

    po::options_description desc;
    ml::add_logging_options(desc.add_options());

    CmdLine cmdline{};
    cmdline.add_argument("--log-level=input=error");
    cmdline.add_argument("--log-level=wayland=info");

    mir::options::ProgramOption mo;
    mo.parse_arguments(desc, cmdline.argc(), cmdline.argv());

    std::string message = "hello";

    EXPECT_CALL(*logger, log(ml::Severity::error, message, "input")).Times(1);
    EXPECT_CALL(*logger, log(ml::Severity::warning, message, "input")).Times(0);
    EXPECT_CALL(*logger, log(ml::Severity::informational, message, "wayland")).Times(1);
    EXPECT_CALL(*logger, log(ml::Severity::debug, message, "wayland")).Times(0);

    mir::log_error({ml::input()}, message);
    mir::log_warning({ml::input()}, message);

    mir::log_info({ml::wayland()}, message);
    mir::log_debug({ml::wayland()}, message);
}
