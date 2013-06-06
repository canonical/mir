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

#include "mir/logging/input_report.h"
#include "mir/logging/logger.h"

#include <std/Log.h>

#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ml  = mir::logging;
namespace mli = mir::logging::legacy_input_report;

namespace
{
class MockLogger : public ml::Logger
{
public:
    MOCK_METHOD3(log, void(Severity severity, const std::string& message, const std::string& component));
    ~MockLogger() noexcept(true) {}
};

struct InputReport : public testing::Test
{
    MockLogger     logger;

    InputReport()
    {
        mli::initialize(mir::test::fake_shared(logger));
    }
};

char const* const component = "android-input";
char const* const LOG_TAG = 0;
}

TEST_F(InputReport, debug_message)
{
    EXPECT_CALL(logger, log(
            ml::Logger::debug,
            testing::HasSubstr(__PRETTY_FUNCTION__),
            component));

  ALOG(LOG_DEBUG, NULL, "Test function is %s", __PRETTY_FUNCTION__);
}

TEST_F(InputReport, unknown_message)
{
    char const* const unknown = "Unknown message";

    EXPECT_CALL(logger, log(
            ml::Logger::debug,
            unknown,
            component));

  ALOG(LOG_UNKNOWN, NULL, unknown);
}

TEST_F(InputReport, verbose_message)
{
    char const* const verbose = "A very long story. (OK, I lied.)";

    EXPECT_CALL(logger, log(
            ml::Logger::debug,
            verbose,
            component));

  ALOG(LOG_VERBOSE, NULL, verbose);
}

TEST_F(InputReport, info_message)
{
    EXPECT_CALL(logger, log(
            ml::Logger::informational,
            __PRETTY_FUNCTION__,
            component));

    ALOGI(__PRETTY_FUNCTION__);
}

TEST_F(InputReport, warning_message)
{
    EXPECT_CALL(logger, log(
            ml::Logger::warning,
            __PRETTY_FUNCTION__,
            component));

    ALOGW(__PRETTY_FUNCTION__);
}

TEST_F(InputReport, error_message)
{
    EXPECT_CALL(logger, log(
            ml::Logger::error,
            __PRETTY_FUNCTION__,
            component));

    ALOGE(__PRETTY_FUNCTION__);
}
