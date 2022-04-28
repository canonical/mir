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

#ifndef MIR_IMMEDIATE_EXECUTOR_H_
#define MIR_IMMEDIATE_EXECUTOR_H_

#include "executor.h"

namespace mir
{
/// An Executor that runs work on the current thread within spawn()
extern Executor& immediate_executor;
}

#endif // MIR_IMMEDIATE_EXECUTOR_H_
