/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_IPC_CHECKPOINTS_H_
#define MIR_TEST_IPC_CHECKPOINTS_H_

#include <semaphore.h>

#include <initializer_list>
#include <string>

namespace mir_test_framework
{

class IPCCheckpoints
{
public:
    IPCCheckpoints(std::initializer_list<std::string> const& checkpoints);
    virtual ~IPCCheckpoints() = default;
    
    void unblock(std::string const& checkpoint_name);
    void wait_for_at_most_seconds(std::string const& checkpoint_name, int seconds);

protected:
    IPCCheckpoints(const IPCCheckpoints&) = delete;
    IPCCheckpoints& operator=(const IPCCheckpoints&) = delete;
};

ACTION_P2(UnblockCheckpoint, checkpoints, checkpoint)
{
    checkpoints->unblock(checkpoint);
}

}

#endif
