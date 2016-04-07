/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

MirInputConfigurationEvent::MirInputConfigurationEvent() :
    MirEvent(mir_event_type_input_configuration)
{
}

MirInputConfigurationAction MirInputConfigurationEvent::action() const
{
    return action_;
}

void MirInputConfigurationEvent::set_action(MirInputConfigurationAction action)
{
    action_ = action;
}

std::chrono::nanoseconds MirInputConfigurationEvent::when() const
{
    return when_;
}

void MirInputConfigurationEvent::set_when(std::chrono::nanoseconds const& when)
{
    when_ = when;
}

MirInputDeviceId MirInputConfigurationEvent::id() const
{
    return id_;
}

void MirInputConfigurationEvent::set_id(MirInputDeviceId id)
{
    id_ = id;
}
