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

#include <mir/logging/logger.h>
#include <mir/log.h>

#include <chrono>
#include <format>
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
    auto const& tag = ml::core;

    EXPECT_CALL(*logger, log(severity, message, std::string{tag.name()}));

    mir::log(severity, {tag}, message);
}

TEST_F(TestLog, log_calls_logger_with_colon_separated_component_for_multiple_tags)
{
    auto const severity = mir::logging::Severity::debug;
    auto const message = "A message, and a system of messages";
    std::string expected_component = std::string{ml::graphics.name()} + ":" + std::string{ml::wayland.name()};


    EXPECT_CALL(*logger, log(severity, message, expected_component));

    mir::log(severity, {ml::graphics, ml::wayland}, message);
}

TEST_F(TestLog, log_debug_works)
{
    auto constexpr const message = "Twice around the world";
    std::string const expected_component{ml::core.name()};

    EXPECT_CALL(*logger, log(ml::Severity::debug, message, expected_component));

    mir::log_debug({ml::core}, message);
}

TEST_F(TestLog, log_info_works)
{
    auto constexpr const message = "Arbitrarium";
    std::string const expected_component{ml::core.name()};

    EXPECT_CALL(*logger, log(ml::Severity::informational, message, expected_component));

    mir::log_info({ml::core}, message);
}

TEST_F(TestLog, log_warning_works)
{
    auto constexpr const message = "Abort! Abort!";
    std::string const expected_component{ml::core.name()};

    EXPECT_CALL(*logger, log(ml::Severity::warning, message, expected_component));

    mir::log_warning({ml::core}, message);
}

TEST_F(TestLog, log_error_works)
{
    auto constexpr const message = "Terrain warning";
    std::string const expected_component{ml::core.name()};

    EXPECT_CALL(*logger, log(ml::Severity::error, message, expected_component));

    mir::log_error({ml::core}, message);
}

TEST_F(TestLog, log_critical_works)
{
    auto constexpr const message = "ENOCAKE";
    std::string const expected_component{ml::core.name()};

    EXPECT_CALL(*logger, log(ml::Severity::critical, message, expected_component));

    mir::log_critical({ml::core}, message);
}

TEST_F(TestLog, can_use_format_string)
{
    auto const severity = mir::logging::Severity::informational;
    constexpr std::string_view fmt_string = "now: {}, then: {:10.5f}, again: {:.{}}";
    auto const& tag = ml::input;

    auto const now = std::chrono::system_clock::now();
    auto const a_float = 3.1415f;
    auto const a_stringish = "Merrily, merrily, merrily, merrily";

    auto const message = std::format(fmt_string, now, a_float, a_stringish, strlen("Merrily"));

    EXPECT_CALL(*logger, log(severity, message, std::string{tag.name()}));

    mir::log(severity, {tag}, message);
}
