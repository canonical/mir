/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/events/input_configuration_event.h"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirInputConfigurationEvent::MirInputConfigurationEvent()
{
    event.initInputConfiguration();
}

MirInputConfigurationAction MirInputConfigurationEvent::action() const
{
    return static_cast<MirInputConfigurationAction>(event.asReader().getInputConfiguration().getAction());
}

void MirInputConfigurationEvent::set_action(MirInputConfigurationAction action)
{
    event.getInputConfiguration().setAction(static_cast<mir::capnp::InputConfigurationEvent::Action>(action));
}

std::chrono::nanoseconds MirInputConfigurationEvent::when() const
{
    return std::chrono::nanoseconds{event.asReader().getInputConfiguration().getWhen().getCount()};
}

void MirInputConfigurationEvent::set_when(std::chrono::nanoseconds const& when)
{
    event.getInputConfiguration().getWhen().setCount(when.count());
}

MirInputDeviceId MirInputConfigurationEvent::id() const
{
    return event.asReader().getInputConfiguration().getId().getId();
}

void MirInputConfigurationEvent::set_id(MirInputDeviceId id)
{
    event.getInputConfiguration().getId().setId(id);
}
