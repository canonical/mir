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
#include <sys/mman.h>

namespace mtf = mir_test_framework;

mtf::IPCSemaphore::IPCSemaphore()
{
    static int const semaphore_enable_process_shared = 1;

    sem = static_cast<sem_t*>(mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_SHARED, 0, 0));
    sem_init(sem, semaphore_enable_process_shared, 0);
}

mtf::IPCSemaphore::~IPCSemaphore() noexcept(true)
{
    sem_destroy(sem);
    munmap(sem, sizeof(sem_t));
}

void mtf::IPCSemaphore::wake_up_everyone()
{
    sem_post(sem);
}

void mtf::IPCSemaphore::wait_for_at_most_seconds(int seconds)
{
    struct timespec abs_timeout = { time(NULL) + seconds, 0 };
    sem_timedwait(sem, &abs_timeout);
}
