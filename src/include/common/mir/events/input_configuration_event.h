/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_COMMON_INPUT_CONFIGURATION_EVENT_H_
#define MIR_COMMON_INPUT_CONFIGURATION_EVENT_H_

#include <chrono>

#include "mir/events/event.h"

struct MirInputConfigurationEvent : MirEvent
{
    MirInputConfigurationEvent();

    MirInputConfigurationAction action() const;
    void set_action(MirInputConfigurationAction action);

    std::chrono::nanoseconds when() const;
    void set_when(std::chrono::nanoseconds const& when);

    MirInputDeviceId id() const;
    void set_id(MirInputDeviceId id);

private:
    MirInputConfigurationAction action_;
    std::chrono::nanoseconds when_{0};
    MirInputDeviceId id_{0};
};

#endif /* MIR_COMMON_INPUT_CONFIGURATION_EVENT_H_ */
