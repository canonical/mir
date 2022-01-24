/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

#ifndef MIR_SYSTEM_EXECUTOR_H_
#define MIR_SYSTEM_EXECUTOR_H_

#include "mir/executor.h"

namespace mir
{
class SystemExecutor : public Executor
{
public:
    void spawn(std::function<void()>&& work) override;
};

SystemExecutor system_executor;
}

#endif //MIR_SYSTEM_EXECUTOR_H_
