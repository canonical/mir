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

#include "mir_test_framework/ipc_semaphore.h"

#include <time.h>

namespace mtf = mir_test_framework;

mtf::IPCSemaphore::IPCSemaphore()
{
    static int const semaphore_enable_process_shared = 1;

    sem_init(&sem, semaphore_enable_process_shared, 0);
}

mtf::IPCSemaphore::~IPCSemaphore()
{
    sem_destroy(&sem);
}

void mtf::IPCSemaphore::wake_up_everyone()
{
    sem_post(&sem);
}

void mtf::IPCSemaphore::wait_for_at_most_seconds(int seconds)
{
    struct timespec abs_timeout = { time(NULL) + seconds, 0 };
    sem_timedwait(&sem, &abs_timeout);
}
