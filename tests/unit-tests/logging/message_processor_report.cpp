/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/logging/message_processor_report.h"
#include "mir/logging/logger.h"

#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

namespace
{
class MockTimeSource : public mir::time::TimeSource
{
public:
    MOCK_CONST_METHOD0(sample, mir::time::Timestamp());

    ~MockTimeSource() noexcept(true) {}
};

class MockLogger : public mir::logging::Logger
{
public:
    MOCK_METHOD3(log, void(Severity severity, const std::string& message, const std::string& component));
};

struct MessageProcessorReportTest : public Test
{
    MockLogger     logger;
    MockTimeSource time_source;

    mir::logging::MessageProcessorReport report;

    MessageProcessorReportTest() :
        report(mir::test::fake_shared(logger), mir::test::fake_shared(time_source)) {}
};
}

TEST_F(MessageProcessorReportTest, everything_fine)
{
    mir::time::Timestamp a_time;
    EXPECT_CALL(time_source, sample()).Times(2).WillRepeatedly(Return(a_time));
    EXPECT_CALL(logger, log(
        mir::logging::Logger::informational,
        EndsWith(": a_function(), elapsed=0µs"),
        "frontend::MessageProcessor")).Times(1);

    report.received_invocation(this, 1, "a_function");
    report.completed_invocation(this, 1, true);
}

TEST_F(MessageProcessorReportTest, slow_call)
{
    mir::time::Timestamp a_time;
    mir::time::Timestamp another_time = a_time + std::chrono::microseconds(1234);

    EXPECT_CALL(time_source, sample()).Times(2)
    .WillOnce(Return(a_time)).WillOnce(Return(another_time));

    EXPECT_CALL(logger, log(
        mir::logging::Logger::informational,
        EndsWith("elapsed=1234µs"),
        "frontend::MessageProcessor")).Times(1);

    report.received_invocation(this, 1, __PRETTY_FUNCTION__);
    report.completed_invocation(this, 1, true);
}

TEST_F(MessageProcessorReportTest, reports_disconnect)
{
    mir::time::Timestamp a_time;
    EXPECT_CALL(time_source, sample()).Times(2).WillRepeatedly(Return(a_time));
    EXPECT_CALL(logger, log(
        mir::logging::Logger::informational,
        HasSubstr("(disconnecting)"),
        "frontend::MessageProcessor")).Times(1);

    report.received_invocation(this, 1, __PRETTY_FUNCTION__);
    report.completed_invocation(this, 1, false);
}

TEST_F(MessageProcessorReportTest, reports_error_during_call)
{
    const char* testError = "***Test error***";

    mir::time::Timestamp a_time;
    EXPECT_CALL(time_source, sample()).Times(2).WillRepeatedly(Return(a_time));
    EXPECT_CALL(logger, log(
        mir::logging::Logger::informational,
        HasSubstr(testError),
        "frontend::MessageProcessor")).Times(1);

    report.received_invocation(this, 1, __PRETTY_FUNCTION__);
    report.exception_handled(this, 1, std::runtime_error(testError));
    report.completed_invocation(this, 1, false);
}

TEST_F(MessageProcessorReportTest, reports_unknown_method)
{
    EXPECT_CALL(time_source, sample()).Times(0);
    EXPECT_CALL(logger, log(
        mir::logging::Logger::warning,
        HasSubstr("UNKNOWN method=\"unknown_function_name\""),
        "frontend::MessageProcessor")).Times(1);

    report.unknown_method(this, 1, "unknown_function_name");
}

TEST_F(MessageProcessorReportTest, reports_error_deserializing_call)
{
    const char* testError = "***Test error***";

    EXPECT_CALL(logger, log(
        mir::logging::Logger::informational,
        HasSubstr(testError),
        "frontend::MessageProcessor")).Times(1);

    report.exception_handled(this, std::runtime_error(testError));
}

