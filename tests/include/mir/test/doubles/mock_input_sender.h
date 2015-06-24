/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_SENDER_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_SENDER_H_

#include "mir/input/input_sender.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockInputSender : mir::input::InputSender
{
    MOCK_METHOD1(set_send_observer, void(mir::input::InputSendObserver *));
    MOCK_METHOD2(send_event, void(MirEvent const& ev, std::shared_ptr<mir::input::InputChannel> const& channel));
};

}
}
}

#endif
