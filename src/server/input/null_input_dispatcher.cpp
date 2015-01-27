/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "null_input_dispatcher.h"

namespace mi = mir::input;

void mi::NullInputDispatcher::configuration_changed(nsecs_t /*when*/)
{
}

void mi::NullInputDispatcher::device_reset(int32_t /*device_id*/, nsecs_t /*when*/)
{
}

void mi::NullInputDispatcher::dispatch(MirEvent const& /*event*/)
{
}

void mi::NullInputDispatcher::start()
{
}

void mi::NullInputDispatcher::stop()
{
}

