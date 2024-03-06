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

#ifndef MIR_TEST_DOUBLES_MOCK_SERVER_STATUS_LISTENER_H_
#define MIR_TEST_DOUBLES_MOCK_SERVER_STATUS_LISTENER_H_

#include "mir/server_status_listener.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockServerStatusListener : public mir::ServerStatusListener
{
public:
    MOCK_METHOD(void, paused, (), (override));
    MOCK_METHOD(void, resumed, (), (override));
    MOCK_METHOD(void, started, (), (override));
    MOCK_METHOD(void, ready_for_user_input, (), (override));
    MOCK_METHOD(void, stop_receiving_input, (), (override));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_SERVER_STATUS_LISTENER_H_ */
