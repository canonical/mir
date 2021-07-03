/*
 * client_types.h: Type definitions used in client apps and libmirclient.
 *
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_TOOLKIT_CLIENT_TYPES_H_
#define MIR_TOOLKIT_CLIENT_TYPES_H_

#include <mir_toolkit/events/event.h>
#include <mir_toolkit/common.h>

#include <stddef.h>

#ifdef __cplusplus
/**
 * \defgroup mir_toolkit MIR graphics tools API
 * @{
 */
extern "C" {
#endif

/* Display server connection API */
typedef struct MirBlob MirBlob;

typedef struct MirRectangle
{
    int left;
    int top;
    unsigned int width;
    unsigned int height;
} MirRectangle;

typedef struct MirInputConfig MirInputConfig;
typedef struct MirInputDevice MirInputDevice;
typedef struct MirKeyboardConfig MirKeyboardConfig;
typedef struct MirPointerConfig MirPointerConfig;
typedef struct MirTouchpadConfig MirTouchpadConfig;
typedef struct MirTouchscreenConfig MirTouchscreenConfig;

/**
 * Specifies the origin of an error.
 *
 * This is required to interpret the other aspects of a MirError.
 */
typedef enum MirErrorDomain
{
    /**
     * Errors relating to display configuration.
     *
     * Associated error codes are found in \ref MirDisplayConfigurationError.
     */
    mir_error_domain_display_configuration,
    /**
     * Errors relating to input configuration.
     *
     * Associated error codes are found in \ref MirInputConfigurationError.
     */
    mir_error_domain_input_configuration,
} MirErrorDomain;

/**
 * Errors from the \ref mir_error_domain_display_configuration \ref MirErrorDomain
 */
typedef enum MirDisplayConfigurationError {
    /**
     * Client is not permitted to change global display configuration
     */
    mir_display_configuration_error_unauthorized,
    /**
     * A global configuration change request is already pending
     */
    mir_display_configuration_error_in_progress,
    /**
     * A cancel request was received, but no global display configuration preview is in progress
     */
    mir_display_configuration_error_no_preview_in_progress,
    /**
     * Display configuration was attempted but was rejected by the hardware
     */
     mir_display_configuration_error_rejected_by_hardware
} MirDisplayConfigurationError;

/**
 * Errors from the \ref mir_error_domain_input_configuration \ref MirErrorDomain
 */
typedef enum MirInputConfigurationError {
    /**
     * Input configuration was attempted but was rejected by driver
     */
     mir_input_configuration_error_rejected_by_driver,

    /**
     * Client is not permitted to change global input configuration
    */
     mir_input_configuration_error_base_configuration_unauthorized,

    /**
     * Client is not permitted to change its input configuration
     */
     mir_input_configuration_error_unauthorized,
} MirInputConfigurationError;

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_CLIENT_TYPES_H_ */
