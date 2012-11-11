/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */
#include "mir_test/semaphore.h"

#include <time.h>

mir::Semaphore::Semaphore(int value)
{
    sem_init(&sem, false, value);
}

mir::Semaphore::~Semaphore()
{
}

void mir::Semaphore::wait()
{
    sem_wait(&sem);
}

void mir::Semaphore::post()
{
    sem_post(&sem);
}

// This is awkward but so is chrono.h's attempt to alias boost in to the std namespace. Awful things happen.
void mir::Semaphore::wait_seconds(int seconds)
{
    struct timespec absolute_time;
    absolute_time.tv_sec = time(NULL) + seconds;
    absolute_time.tv_nsec = 0;
    sem_timedwait(&sem, &absolute_time);
#include <stdio.h>
    printf("error: %s \n", strerror(errno));
}

