/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_MESSAGE_SENDER_H_
#define MIR_TEST_DOUBLES_MOCK_MESSAGE_SENDER_H_

#include "src/server/frontend/message_sender.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
class MockMessageSender : public frontend::MessageSender
{
public:
    MOCK_METHOD3(send, void(char const*, size_t, frontend::FdSets const &));
};
}
}
}

#endif //MIR_TEST_DOUBLES_MOCK_MESSAGE_SENDER_H_
