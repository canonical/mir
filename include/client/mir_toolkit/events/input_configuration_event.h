/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TOOLKIT_EVENTS_INPUT_CONFIGURATION_EVENT_H_
#define MIR_TOOLKIT_EVENTS_INPUT_CONFIGURATION_EVENT_H_

#include <mir_toolkit/events/event.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/// MirInputConfigurationEvent indicates a configuration change in the input device subsystem. Eventually
/// it's usage will be required to properly interpret MirInputEvent, for example:
///    If we receive a button down, and then a device reset, we should not expect
///    to receive the button up.
///
///    Another example, the maximum/minimum axis values for a device may have been reconfigured and
///    need to be required.
///
/// Of course as things stand there is no client input-device introspection API so these events
/// are difficult to use.
    
typedef enum
{
    mir_input_configuration_action_configuration_changed,
    mir_input_configuration_action_device_reset
} MirInputConfigurationAction;

/**
 * Retrieve the input configuration action which occurred.
 *
 * \param[in] ev The input configuration event
 * \return       The action
 */
MirInputConfigurationAction mir_input_configuration_event_get_action(MirInputConfigurationEvent const* ev);

/**
 * Retreive the time associated with a MirInputConfiguration event

 * \param[in] ev The input configuration event
 * \return       The time in nanoseconds since epoch
 */
int64_t mir_input_configuration_event_get_time(MirInputConfigurationEvent const* ev);

/**
 * Retreive the device id associated with a MirInputConfiguration event

 * \param[in] ev The input configuration event
 * \return       The device id or -1 if not applicable to events of this action
 */
MirInputDeviceId mir_input_configuration_event_get_device_id(MirInputConfigurationEvent const* ev);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_INPUT_CONFIGURATION_EVENT_H_ */
