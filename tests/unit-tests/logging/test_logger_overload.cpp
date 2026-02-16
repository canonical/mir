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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdarg>

namespace ml = mir::logging;
using namespace testing;

namespace
{
class RecordingLogger : public ml::Logger
{
public:
    MOCK_METHOD(void, log, (ml::Severity severity, const std::string& message, const std::string& component), (override));
    
    // Provide implementation for the deprecated variadic method
    void log(char const* component, ml::Severity severity, char const* format, ...) override
    {
        char message[4096];
        va_list va;
        va_start(va, format);
        vsnprintf(message, sizeof(message), format, va);
        va_end(va);
        log(severity, std::string{message}, std::string{component});
    }
};

struct LoggerOverloadTest : public testing::Test
{
    std::shared_ptr<RecordingLogger> logger{std::make_shared<RecordingLogger>()};
};
}

TEST_F(LoggerOverloadTest, no_args_format_string_uses_new_api)
{
    EXPECT_CALL(*logger, log(ml::Severity::informational, "Simple message", "component"));
    
    logger->log("component", ml::Severity::informational, "Simple message");
}

TEST_F(LoggerOverloadTest, single_arg_format_string_uses_new_api)
{
    EXPECT_CALL(*logger, log(ml::Severity::informational, "Value: 42", "component"));
    
    logger->log("component", ml::Severity::informational, "Value: {}", 42);
}

TEST_F(LoggerOverloadTest, multiple_args_format_string_uses_new_api)
{
    EXPECT_CALL(*logger, log(ml::Severity::informational, "Values: 42 and hello", "component"));
    
    logger->log("component", ml::Severity::informational, "Values: {} and {}", 42, "hello");
}

TEST_F(LoggerOverloadTest, old_printf_style_still_works)
{
    EXPECT_CALL(*logger, log(ml::Severity::informational, "Value: 42", "component"));
    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    logger->log("component", ml::Severity::informational, "Value: %d", 42);
    #pragma GCC diagnostic pop
}
