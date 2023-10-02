/*
 * Copyright © Canonical Ltd.
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
 */

#include "input_platform.h"
#include <mir/dispatch/action_queue.h>

namespace mi = mir::input;
namespace miv = mir::input::virt;

miv::VirtualInputPlatform::VirtualInputPlatform()
{
}

std::shared_ptr<mir::dispatch::Dispatchable> miv::VirtualInputPlatform::dispatchable()
{
    return std::make_shared<mir::dispatch::ActionQueue>();
}

void miv::VirtualInputPlatform::start()
{
}

void miv::VirtualInputPlatform::stop()
{
}

void miv::VirtualInputPlatform::pause_for_config()
{
}

void miv::VirtualInputPlatform::continue_after_config()
{
}