/*
 * Copyright © 2014-2016 Canonical Ltd.
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

#ifndef MIR_TOOLKIT_EVENT_H_
#define MIR_TOOLKIT_EVENT_H_

#include <stddef.h>
#include <stdint.h>
#include "mir_toolkit/common.h"

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
typedef enum
{
    mir_event_type_key,
    mir_event_type_motion,
    mir_event_type_surface, /* __attribute__ ((deprecated("use mir_event_type_window instead"))), */
    mir_event_type_window = mir_event_type_surface,
    mir_event_type_resize,
    mir_event_type_prompt_session_state_change,
    mir_event_type_orientation,
    mir_event_type_close_surface, /*__attribute__ ((deprecated("use mir_event_type_close_window instead"))), */
    mir_event_type_close_window = mir_event_type_close_surface,
    /* Type for new style input event will be returned from mir_event_get_type
       when old style event type was mir_event_type_key or mir_event_type_motion */
    mir_event_type_input,
    mir_event_type_keymap,
    mir_event_type_input_configuration,
    mir_event_type_surface_output, /* __attribute__ ((deprecated("use mir_event_type_window_output instead"))), */
    mir_event_type_window_output = mir_event_type_surface_output,
    mir_event_type_input_device_state,
    mir_event_type_surface_placement, /*__attribute__ ((deprecated("use mir_event_type_window_placement instead"))), */
    mir_event_type_window_placement = mir_event_type_surface_placement,
} MirEventType;
#pragma GCC diagnostic pop

typedef struct MirSurfaceEvent MirSurfaceEvent; /* __attribute__ ((deprecated("use MirWindowEvent instead"))); */
typedef struct MirSurfaceEvent MirWindowEvent;
typedef struct MirResizeEvent MirResizeEvent;
typedef struct MirPromptSessionEvent MirPromptSessionEvent;
typedef struct MirOrientationEvent MirOrientationEvent;
typedef struct MirCloseSurfaceEvent MirCloseSurfaceEvent; /* __attribute__ ((deprecated("use MirCloseWindowEvent instead"))); */
typedef struct MirCloseSurfaceEvent MirCloseWindowEvent;
typedef struct MirInputEvent MirInputEvent;
typedef struct MirKeymapEvent MirKeymapEvent;
typedef struct MirInputConfigurationEvent MirInputConfigurationEvent;
typedef struct MirSurfaceOutputEvent MirSurfaceOutputEvent;
typedef struct MirInputDeviceStateEvent MirInputDeviceStateEvent;
typedef struct MirSurfacePlacementEvent MirSurfacePlacementEvent;

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
MirSurfaceEvent const* mir_event_get_surface_event(MirEvent const* event);
/* __attribute__ ((deprecated("use mir_event_get_window_event instead"))); */

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
__attribute__ ((deprecated))
/// @endcond
MirCloseSurfaceEvent const* mir_event_get_close_surface_event(MirEvent const* event);

/**
 * Retrieve the MirKeymapEvent associated with a MirEvent of
 * type mir_event_type_keymap. The event signifies that the keymap
 * applied for the relevant surface has changed.
 *
 * \param [in] event The event
 * \return           The associated MirKeymapEvent
 */
MirKeymapEvent const* mir_event_get_keymap_event(MirEvent const* event);

/**
 * Retrieve the MirInputConfiguration associated with a MirEvent of
 * type mir_event_type_input_configuration. The event signifies that the
 * input device configuration has changed.
 *
 * \deprecated Input devices and changes to the input devices are indicated
 * via the mir_input_config_change_callback
 * \param [in] event The event
 * \return           The associated MirInputConfigurationEvent
 */
/// @cond
__attribute__((deprecated))
/// @endcond
MirInputConfigurationEvent const* mir_event_get_input_configuration_event(MirEvent const* event);

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
MirSurfaceOutputEvent const* mir_event_get_surface_output_event(MirEvent const* event);

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
MirSurfacePlacementEvent const* mir_event_get_surface_placement_event(MirEvent const* event);

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
