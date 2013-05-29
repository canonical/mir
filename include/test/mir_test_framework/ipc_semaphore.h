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

#ifndef MIR_TEST_IPC_SEMAPHORE_H_
#define MIR_TEST_IPC_SEMAPHORE_H_

#include <semaphore.h>

#include <initializer_list>
#include <string>

namespace mir_test_framework
{

class IPCSemaphore
{
public:
    IPCSemaphore();
    virtual ~IPCSemaphore();
    
    void wake_up_everyone();
    void wait_for_at_most_seconds(int seconds);

protected:
    IPCSemaphore(const IPCSemaphore&) = delete;
    IPCSemaphore& operator=(const IPCSemaphore&) = delete;

private:
    sem_t* sem;
};

}

#endif
