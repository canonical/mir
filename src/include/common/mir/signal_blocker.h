/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_SIGNAL_BLOCKER_H_
#define MIR_SIGNAL_BLOCKER_H_

#include <signal.h>

namespace mir
{
class SignalBlocker
{
public:
    SignalBlocker();
    ~SignalBlocker() noexcept(false);

    SignalBlocker(SignalBlocker const&) = delete;
    SignalBlocker& operator=(SignalBlocker const&) = delete;
private:
    sigset_t previous_set;
};
}

#endif /* MIR_SIGNAL_BLOCKER_H_ */
