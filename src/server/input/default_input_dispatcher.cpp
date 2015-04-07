/*
 * Copyright © 2015 Canonical Ltd.
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

#include "default_input_dispatcher.h"

namespace mi = mir::input;

mi::DefaultInputDispatcher::DefaultInputDispatcher(std::shared_ptr<mi::Scene> const& scene)
    : scene(scene)
{
    (void) scene;
}

void mi::DefaultInputDispatcher::configuration_changed(std::chrono::nanoseconds when)
{
    (void) when;
}

void mi::DefaultInputDispatcher::device_reset(int32_t device_id, std::chrono::nanoseconds when)
{
    (void) device_id;
    (void) when;
}

bool mi::DefaultInputDispatcher::dispatch(MirEvent const& event)
{
    (void) event;
    return true;
}

void mi::DefaultInputDispatcher::start()
{
}

void mi::DefaultInputDispatcher::stop()
{
}

void mi::DefaultInputDispatcher::set_focus(std::shared_ptr<input::Surface> const& target)
{
    (void) target;
}

