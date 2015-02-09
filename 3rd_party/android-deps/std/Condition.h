/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_ANDROID_UBUNTU_CONDITION_H_
#define MIR_ANDROID_UBUNTU_CONDITION_H_

#include <std/Timers.h>

#include <condition_variable>
#include <chrono>

namespace mir_input
{
typedef std::condition_variable_any Condition;

inline void broadcast(Condition& c) { c.notify_all(); }

template <typename Lock>
inline void waitRelative(Condition& c, Lock& l, std::chrono::nanoseconds reltime)
{
    c.wait_for(l, std::chrono::nanoseconds(reltime));
}
}

namespace android
{
using ::mir_input::Condition;
using ::mir_input::broadcast;
using ::mir_input::waitRelative;
}

#endif /* MIR_ANDROID_UBUNTU_CONDITION_H_ */
