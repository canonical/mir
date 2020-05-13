/*
 * Copyright © 2014-2020 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TOOLKIT_EVENT_H_
#define MIR_TOOLKIT_EVENT_H_

#include <mir_toolkit/events/enums.h>

#include <stddef.h>
#include <stdint.h>
#include "mir_toolkit/common.h"
#include <mir_toolkit/deprecations.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

typedef struct MirSurfaceEvent MirSurfaceEvent
    MIR_FOR_REMOVAL_IN_VERSION_1("use MirWindowEvent instead");
typedef struct MirSurfaceEvent MirWindowEvent;
typedef struct MirResizeEvent MirResizeEvent;
typedef struct MirPromptSessionEvent MirPromptSessionEvent;
typedef struct MirOrientationEvent MirOrientationEvent;
typedef struct MirCloseSurfaceEvent MirCloseSurfaceEvent
    MIR_FOR_REMOVAL_IN_VERSION_1("use MirCloseWindowEvent instead");
typedef struct MirCloseSurfaceEvent MirCloseWindowEvent;
typedef struct MirInputEvent MirInputEvent;
typedef struct MirKeymapEvent MirKeymapEvent;
typedef struct MirInputConfigurationEvent MirInputConfigurationEvent 
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirInputDeviceStateEvent and the MirInputConfig callback instead");
typedef struct MirSurfaceOutputEvent MirSurfaceOutputEvent
    MIR_FOR_REMOVAL_IN_VERSION_1("use MirWindowOutputEvent instead");
typedef struct MirSurfaceOutputEvent MirWindowOutputEvent;
typedef struct MirInputDeviceStateEvent MirInputDeviceStateEvent;
typedef struct MirSurfacePlacementEvent MirSurfacePlacementEvent
    MIR_FOR_REMOVAL_IN_VERSION_1("use MirWindowPlacementEvent instead");
typedef struct MirSurfacePlacementEvent MirWindowPlacementEvent;

typedef struct MirCookie MirCookie;

typedef struct MirEvent MirEvent;

#ifdef __cplusplus
}
/**@}*/
#endif

#include "mir_toolkit/events/input/input_event.h"
#include "mir_toolkit/events/resize_event.h"
#include "mir_toolkit/events/surface_event.h"
#include "mir_toolkit/events/window_event.h"
#include "mir_toolkit/events/orientation_event.h"
#include "mir_toolkit/events/prompt_session_event.h"
#include "mir_toolkit/events/keymap_event.h"
#include "mir_toolkit/events/input_configuration_event.h"
#include "mir_toolkit/events/surface_output_event.h"
#include "mir_toolkit/events/window_output_event.h"
#include "mir_toolkit/events/input_device_state_event.h"

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Retrieves the type of a MirEvent. Now preferred over direct access to ev->type.
 * In particular ev->type will never be mir_event_type_input and mir_event_get_type
 * is the only way to ensure mir_event_get_input_event will succeed.
 *
 * \param [in] event The event
 * \return           The event type
 */
MirEventType mir_event_get_type(MirEvent const* event);

/**
 * Retrieve the MirInputEvent associated with a MirEvent of 
 * type mir_event_type_input. See <mir_toolkit/events/input/input_event.h>
 * for accessors.
 *
 * \param [in] event The event
 * \return           The associated MirInputEvent
 */
MirInputEvent const* mir_event_get_input_event(MirEvent const* event);

/**
 * Retrieve the MirSurfaceEvent associated with a MirEvent of
 * type mir_event_type_surface. See <mir_toolkit/events/surface_event.h>
 * for accessors.
 *
 * \param [in] event The event
 * \return           The associated MirSurfaceEvent
 */
MirSurfaceEvent const* mir_event_get_surface_event(MirEvent const* event)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_event_get_window_event instead");

/**
 * Retrieve the MirWindowEvent associated with a MirEvent of
 * type mir_event_type_window. See <mir_toolkit/events/surface_event.h>
 * for accessors.
 *
 * \param [in] event The event
 * \return           The associated MirWindowEvent
 */
MirWindowEvent const* mir_event_get_window_event(MirEvent const* event);

/**
 * Retrieve the MirResizeEvent associated with a MirEvent of
 * type mir_event_type_resize. See <mir_toolkits/events/resize_event.h>
 * for accessors.
 *
 * \param [in] event The event
 * \return           The associated MirResizeEvent
 */
MirResizeEvent const* mir_event_get_resize_event(MirEvent const* event);

/**
 * Retrieve the MirPromptSessionEvent associated with a MirEvent of
 * type mir_event_type_prompt_session_state_change. See <mir_toolkits/events/prompt_session_event.h>
 * for accessors.
 *
 * \param [in] event The event
 * \return           The associated MirPromptSessionEvent
 */
MirPromptSessionEvent const* mir_event_get_prompt_session_event(MirEvent const* event);

/**
 * Retrieve the MirOrientationEvent associated with a MirEvent of
 * type mir_event_type_orientation. See <mir_toolkit/events/orientation_event.h>
 * for accessors.
 *
 * \param [in] event The event
 * \return           The associated MirOrientationEvent
 */
MirOrientationEvent const* mir_event_get_orientation_event(MirEvent const* event);

// Ignore use of deprecate MirCloseSurfaceEvent typedef in deprecated function (for now)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
/**
 * Retrieve the MirCloseSurfaceEvent associated with a MirEvent of
 * type mir_event_type_close_surface. The event is a request to close
 * the surface it is delivered to and has no accessors.
 *
 * \deprecated Use of this function is pointless as there is no way to use the
 * return value.
 *
 * \todo This should be removed from the public API at the next API break.
 *
 * \param [in] event The event
 * \return           The associated MirCloseSurfaceEvent
 */
/// @cond
MIR_FOR_REMOVAL_IN_VERSION_1("Use of this function is pointless as there is no way to use the return value")
/// @endcond
MirCloseSurfaceEvent const* mir_event_get_close_surface_event(MirEvent const* event);
#pragma GCC diagnostic pop

/**
 * Retrieve the MirKeymapEvent associated with a MirEvent of
 * type mir_event_type_keymap. The event signifies that the keymap
 * applied for the relevant surface has changed.
 *
 * \param [in] event The event
 * \return           The associated MirKeymapEvent
 */
MirKeymapEvent const* mir_event_get_keymap_event(MirEvent const* event);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
/**
 * Retrieve the MirInputConfig associated with a MirEvent of
 * type mir_event_type_input_configuration. The event signifies that the
 * input device configuration has changed.
 *
 * \deprecated Input devices and changes to the input devices are indicated
 * via the MirInputConfigCallback
 * \param [in] event The event
 * \return           The associated MirInputConfigurationEvent
 */
/// @cond
MIR_FOR_REMOVAL_IN_VERSION_1("Input devices and changes to the input devices are indicated via the MirInputConfigCallback")
/// @endcond
MirInputConfigurationEvent const* mir_event_get_input_configuration_event(MirEvent const* event);
#pragma GCC diagnostic pop

/**
 * Retrieve the MirSurfaceOutputEvent associated with a MirEvent of type
 * mir_event_type_surface_output. The event signifies that the properties
 * of the output the surface is displayed upon have changed.
 *
 * A MirSurfaceOutputEvent is generated either when the properties of the
 * output the surface is primarily on change (for example: by user configuration
 * of resolution) or when the output the surface is primarily on changes
 * (for example: when a user moves the surface from one monitor to another).
 *
 * \param [in] event The event
 * \return           The associated MirSurfaceOutputEvent
 */
MirSurfaceOutputEvent const* mir_event_get_surface_output_event(MirEvent const* event)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_event_get_window_output_event");

/**
 * Retrieve the MirWindowOutputEvent associated with a MirEvent of type
 * mir_event_type_window_output. The event signifies that the properties
 * of the output the window is displayed upon have changed.
 *
 * A MirWindowOutputEvent is generated either when the properties of the
 * output the window is primarily on change (for example: by user configuration
 * of resolution) or when the output the window is primarily on changes
 * (for example: when a user moves the window from one monitor to another).
 *
 * \param [in] event The event
 * \return           The associated MirWindowOutputEvent
 */
MirWindowOutputEvent const* mir_event_get_window_output_event(MirEvent const* event);

/**
 * Retrieve the MirInputDeviceStateEvent associated with a MirEvent of
 * type mir_event_type_input_device_state. The event signifies that the
 * client has not received the most recent input events, and thus receives
 * a state update for all attached devices.
 *
 * \param [in] event The event
 * \return           The associated MirInputConfigurationEvent
 */
MirInputDeviceStateEvent const* mir_event_get_input_device_state_event(MirEvent const* event);

/**
 * Retrieve the MirSurfacePlacementEvent associated with a MirEvent of
 * type mir_event_type_surface_placement. The event signifies that the
 * the server has fulfilled a request for relative surface placement.
 *
 * \param [in] event The event
 * \return           The associated MirSurfacePlacementEvent
 */
MirSurfacePlacementEvent const* mir_event_get_surface_placement_event(MirEvent const* event)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_event_get_window_placement_event");

/**
 * Retrieve the MirWindowPlacementEvent associated with a MirEvent of
 * type mir_event_type_window_placement. The event signifies that the
 * the server has fulfilled a request for relative window placement.
 *
 * \param [in] event The event
 * \return           The associated MirWindowPlacementEvent
 */
MirWindowPlacementEvent const* mir_event_get_window_placement_event(MirEvent const* event);

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * _________________________
 *< Don't use mir_event_ref >
 * -------------------------
 *       \   ^__^
 *        \  (oo)\_______
 *           (__)\       )\/\
 *                ||----w |
 *                ||     || 
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * NOTICE: mir_event_ref and mir_event_unref are implemented in terms of copy
 * until such time that direct MirEvent access is deprecated. This means you
 * must use the return value as your new reference.
 */

/**
 * Reference this MirEvent and return a pointer to the
 * newly referenced instance
 *
 * \param[in] event The event to reference
 * \return          The event pointer to now use
 */
MirEvent const* mir_event_ref(MirEvent const* event) __attribute__((warn_unused_result));

/**
 * Release a reference to a MirEvent.
 *
 * \param[in] event The event to un-reference
 */
void mir_event_unref(MirEvent const* event);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_EVENT_H_ */
