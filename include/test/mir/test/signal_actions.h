/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIR_TEST_SIGNAL_ACTIONS_H_
#define MIR_TEST_SIGNAL_ACTIONS_H_

#include <mir/test/signal.h>

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
ACTION_P(ReturnFalseAndWakeUp, signal)
{
    signal->raise();
    return false;
}
ACTION_P(WakeUp, signal)
{
    signal->raise();
}
ACTION_P2(WakeUpWhenZero, signal, atomic_int)
{
    if (atomic_int->fetch_sub(1) == 1)
    {
        signal->raise();
    }
}
}
}


#endif //MIR_TEST_SIGNAL_ACTIONS_H_
