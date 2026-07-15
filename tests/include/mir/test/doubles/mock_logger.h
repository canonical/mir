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

#ifndef MIR_TEST_DOUBLES_MOCK_LOGGER_H_
#define MIR_TEST_DOUBLES_MOCK_LOGGER_H_

#include <mir/logging/logger.h>
#include <mir/logging/event.h>

#include <gmock/gmock.h>

#include <string>

namespace mir
{
namespace test
{
namespace doubles
{

MATCHER_P2(LogMatching, severity_matcher, message_matcher, "")
{
    return testing::ExplainMatchResult(severity_matcher, arg.severity(), result_listener) &&
           testing::ExplainMatchResult(message_matcher, std::string{arg.message()}, result_listener);
}

class MockLogger : public logging::Logger
{
public:
    MOCK_METHOD(void, log, (logging::Event const& log_event), (override));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_LOGGER_H_ */
