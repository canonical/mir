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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <semaphore.h>

namespace mir
{
class Semaphore
{
public:
    Semaphore(int value = 0);
    virtual ~Semaphore() ;

    void wait();
    void post();
    
    // This is awkward, but so is chrono.h's attempted to insert in to the std namespace
    void wait_seconds(int seconds);
private:
    sem_t sem;
};
}

namespace
{
  ACTION_P(PostSemaphore, sem) { sem->post(); }
}
