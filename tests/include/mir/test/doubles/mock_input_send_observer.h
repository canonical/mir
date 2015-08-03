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

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_SEND_OBSERVER_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_SEND_OBSERVER_H_

#include "mir/input/input_send_observer.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockInputSendObserver : public mir::input::InputSendObserver
{
    MockInputSendObserver() = default;
    MOCK_METHOD3(send_failed, void(MirEvent const&, mir::input::Surface*, FailureReason));
    MOCK_METHOD3(send_suceeded, void(MirEvent const&, mir::input::Surface*, InputResponse));
    MOCK_METHOD2(client_blocked, void(MirEvent const&, mir::input::Surface*));
};

}
}
}

#endif
