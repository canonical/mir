/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_LINEARISING_EXECUTOR_H_
#define MIR_LINEARISING_EXECUTOR_H_

#include "executor.h"

namespace mir
{
/**
 * An Executor that makes the following concurrency guarantees:
 *
 * 1.   If linearising_executor.spawn(A) happens-before linearising_executor.spawn(B) then
 *      A() happens-before B(). That is, the entire execution of A is completed before B is started.
 * 2.   No work is performed concurrently. For any two calls linearising_executor.spawn(A)
 *      linearising_executor.spawn(B) either A() happens-before B() or B() happens-before A().
 * 3.   Work is deferred; linearising_executor.spawn(A) will not block on the execution of A
 */
extern NonBlockingExecutor& linearising_executor;
}


#endif //MIR_LINEARISING_EXECUTOR_H_
