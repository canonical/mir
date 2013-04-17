/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_LOGGER_H_
#define MIR_TEST_DOUBLES_MOCK_LOGGER_H_

#include "src/client/mir_logger.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockLogger : public mir::client::Logger
{
    mir::client::ConsoleLogger real_logger;

public:
    MockLogger()
    {
        using mir::client::ConsoleLogger;

        ON_CALL(*this, error())
            .WillByDefault(::testing::Invoke(&real_logger, &ConsoleLogger::error));
        EXPECT_CALL(*this, error()).Times(0);

        ON_CALL(*this, debug())
            .WillByDefault(::testing::Invoke(&real_logger, &ConsoleLogger::debug));
        EXPECT_CALL(*this, debug()).Times(testing::AtLeast(0));
    }

    MOCK_METHOD0(error,std::ostream& ());
    MOCK_METHOD0(debug,std::ostream& ());
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_MOCK_LOGGER_H_ */
