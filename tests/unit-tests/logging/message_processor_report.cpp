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
