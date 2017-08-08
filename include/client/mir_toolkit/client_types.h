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
#include <mir_toolkit/deprecations.h>

#include <stddef.h>

#ifdef __cplusplus
/**
 * \defgroup mir_toolkit MIR graphics tools API
 * @{
 */
extern "C" {
#endif

/* Display server connection API */
typedef void* MirEGLNativeWindowType;
typedef void* MirEGLNativeDisplayType;
typedef struct MirConnection MirConnection;
typedef struct MirSurface MirSurface MIR_FOR_REMOVAL_IN_VERSION_1("Use MirWindow instead");
typedef struct MirSurface MirWindow;
typedef struct MirSurfaceSpec MirSurfaceSpec MIR_FOR_REMOVAL_IN_VERSION_1("Use MirWindowSpec instead");
typedef struct MirSurfaceSpec MirWindowSpec;
typedef struct MirScreencast MirScreencast;
typedef struct MirScreencastSpec MirScreencastSpec;
typedef struct MirPromptSession MirPromptSession;
typedef struct MirBufferStream MirBufferStream;
typedef struct MirPersistentId MirPersistentId MIR_FOR_REMOVAL_IN_VERSION_1("Use MirWindowId instead");
typedef struct MirPersistentId MirWindowId;
typedef struct MirBlob MirBlob;
typedef struct MirDisplayConfig MirDisplayConfig;
typedef struct MirError MirError;
typedef struct MirPresentationChain MirPresentationChain;
typedef struct MirBuffer MirBuffer;
typedef struct MirRenderSurface MirRenderSurface;

/**
 * Opaque structure containing cursor parameterization. Create with mir_cursor* family.
 * Used with mir_window_configure_cursor.
 */
typedef struct MirCursorConfiguration MirCursorConfiguration
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_spec_set_cursor_name/mir_window_spec_set_cursor_render_surface instead");

/**
 * Descriptor for an output connection.
 *
 *  Each MirOutput corresponds to a video output. This may be a physical connection on the system,
 *  like HDMI or DisplayPort, or may be a virtual output such as a remote display or screencast display.
 */
typedef struct MirOutput MirOutput;

/**
 * Returned by asynchronous functions. Must not be free'd by
 * callers. See the individual function documentation for information
 * on the lifetime of wait handles.
 */
typedef struct MirWaitHandle MirWaitHandle;

typedef struct MirPlatformMessage MirPlatformMessage;

/**
 * Callback to be passed when issuing a mir_connect request.
 *   \param [in] connection          the new connection
 *   \param [in,out] client_context  context provided by client in calling
 *                                   mir_connect
 */
typedef void (*MirConnectedCallback)(
    MirConnection *connection, void *client_context);
typedef MirConnectedCallback mir_connected_callback
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirConnectedCallback instead");

/**
 * Callback to be passed when calling window functions :
 *   \param [in] window             the window being updated
 *   \param [in,out] client_context  context provided by client in calling
 *                                   mir_connect
 */
typedef void (*MirWindowCallback)(MirWindow *window, void *client_context);

/**
 * Callback to be passed when calling:
 *  - mir_buffer_stream_* functions requiring a callback.
 *   \param [in] stream              the buffer stream being updated
 *   \param [in,out] client_context  context provided by client in calling
 *                                   mir_connect
 */
typedef void (*MirBufferStreamCallback)(
    MirBufferStream *stream, void *client_context);
typedef MirBufferStreamCallback mir_buffer_stream_callback
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirBufferStreamCallback instead");

/**
 * Callback for handling of window events.
 *   \param [in] window     The window on which an event has occurred
 *   \param [in] event       The event to be handled
 *   \param [in,out] context The context provided by client
 */
typedef void (*MirWindowEventCallback)(
    MirWindow* window, MirEvent const* event, void* context);

/**
 * Callback called when a lifecycle event/callback is requested
 * from the running server.
 *   \param [in] connection     The connection associated with the lifecycle event
 *   \param [in] cb             The callback requested
 *   \param [in,out] context    The context provided by the client
 */

typedef void (*MirLifecycleEventCallback)(
    MirConnection* connection, MirLifecycleState state, void* context);
typedef MirLifecycleEventCallback mir_lifecycle_event_callback
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirLifecycleEventCallback instead");

/**
 * Callback called when the server pings for responsiveness testing.
 * \param [in] connection       The connection associated with this ping
 * \param [in] serial           Identifier of this ping, to be passed to
 *                              mir_connection_pong()
 * \param [in,out] context      The context provided by the client
 */
typedef void (*MirPingEventCallback)(
    MirConnection* connection, int32_t serial, void* context);
typedef MirPingEventCallback mir_ping_event_callback
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirPingEventCallback instead");

/**
 * Callback called when a display config change has occurred
 *   \param [in] connection     The connection associated with the display change
 *   \param [in,out] context    The context provided by client
 */

typedef void (*MirDisplayConfigCallback)(
    MirConnection* connection, void* context);
typedef MirDisplayConfigCallback mir_display_config_callback
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirDisplayConfigCallback instead");

/**
 * Callback called when a request for client file descriptors completes
 *   \param [in] prompt_session  The prompt session
 *   \param [in] count          The number of FDs allocated
 *   \param [in] fds            Array of FDs
 *   \param [in,out] context    The context provided by client
 *
 *   \note Copy the FDs as the array will be invalidated after callback completes
 */

typedef void (*MirClientFdCallback)(
    MirPromptSession *prompt_session, size_t count, int const* fds, void* context);
typedef MirClientFdCallback mir_client_fd_callback
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirClientFdCallback instead");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
typedef void (*MirWindowIdCallback)(
    MirWindow* window, MirPersistentId* id, void* context);
#pragma GCC diagnostic pop

/**
 * MirBufferUsage specifies how a surface can and will be used. A "hardware"
 * surface can be used for OpenGL accelerated rendering. A "software" surface
 * is one that can be addressed in main memory and blitted to directly.
 */
typedef enum MirBufferUsage
{
    mir_buffer_usage_hardware = 1,
    mir_buffer_usage_software
} MirBufferUsage MIR_FOR_REMOVAL_IN_VERSION_1("No longer applicable when using MirRenderSurface");

/**
 * MirWindowParameters is the structure of minimum required information that
 * you must provide to Mir in order to create a window.
 */
typedef struct MirSurfaceParameters
{
    char const *name;
    int width;
    int height;
    MirPixelFormat pixel_format;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    MirBufferUsage buffer_usage;
#pragma GCC diagnostic pop

    /**
     * The id of the output to place the surface in.
     *
     * Use one of the output ids from MirDisplayConfiguration/MirDisplayOutput
     * to place a surface on that output. Only fullscreen placements are
     * currently supported. If you don't have special placement requirements,
     * use the value mir_display_output_id_invalid.
     */
    uint32_t output_id;
} MirSurfaceParameters MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_get apis or listen for attribute events instead");

enum { mir_platform_package_max = 32 };

/**
 * The native buffer type for the system the client is connected on
 *
 * \deprecated Use of this type is inherently non-portable in the presence
 * of plug-in platform modules as these need not correspond to the available
 * types.
 * \todo This should be removed from the public API at the next API break.
 */
#ifndef __cplusplus
MIR_FOR_REMOVAL_IN_VERSION_1("Use of this type is inherently non-portable")
#endif
typedef enum MirPlatformType
{
    mir_platform_type_gbm,
    mir_platform_type_android,
    mir_platform_type_eglstream,
} MirPlatformType;

typedef struct MirPlatformPackage
{
    int data_items;
    int fd_items;

    int data[mir_platform_package_max];
    int fd[mir_platform_package_max];
} MirPlatformPackage;

/**
 * Retrieved information about a loadable module. This allows clients to
 * identify the underlying platform. E.g. whether the graphics are
 * "mir:android" or "mir:mesa".
 * Third party graphics platforms do not currently exist but should be
 * named according to the vendor and platform. Vis: "<vendor>:<platform>"
 */
typedef struct MirModuleProperties
{
    char const *name;
    int major_version;
    int minor_version;
    int micro_version;
    char const *filename;
} MirModuleProperties;

typedef enum MirBufferLayout
{
    mir_buffer_layout_unknown = 0,
    mir_buffer_layout_linear  = 1,
} MirBufferLayout;

typedef enum MirPresentMode
{
    mir_present_mode_immediate, //same as VK_PRESENT_MODE_IMMEDIATE_KHR
    mir_present_mode_mailbox, //same as VK_PRESENT_MODE_MAILBOX_KHR
    mir_present_mode_fifo, //same as VK_PRESENT_MODE_FIFO_KHR
    mir_present_mode_fifo_relaxed, //same as VK_PRESENT_MODE_FIFO_RELAXED_KHR
    mir_present_mode_num_modes
} MirPresentMode;

/**
 * Retrieved information about a MirWindow. This is most useful for learning
 * how and where to write to a 'mir_buffer_usage_software' surface.
 */
typedef struct MirGraphicsRegion
{
    int width;
    int height;
    int stride;
    MirPixelFormat pixel_format;
    char *vaddr;

} MirGraphicsRegion;

/**
 * DEPRECATED. use MirDisplayConfiguration
 */
enum { mir_supported_pixel_format_max = 32 };
typedef struct MirDisplayInfo
{
    uint32_t width;
    uint32_t height;

    int supported_pixel_format_items;
    MirPixelFormat supported_pixel_format[mir_supported_pixel_format_max];
} MirDisplayInfo;

/**
 * MirDisplayConfiguration provides details of the graphics environment.
 */

typedef struct MirDisplayCard
{
    uint32_t card_id;
    uint32_t max_simultaneous_outputs;
} MirDisplayCard;

typedef enum MirDisplayOutputType
{
    mir_display_output_type_unknown     = mir_output_type_unknown,
    mir_display_output_type_vga         = mir_output_type_vga,
    mir_display_output_type_dvii        = mir_output_type_dvii,
    mir_display_output_type_dvid        = mir_output_type_dvid,
    mir_display_output_type_dvia        = mir_output_type_dvia,
    mir_display_output_type_composite   = mir_output_type_composite,
    mir_display_output_type_svideo      = mir_output_type_svideo,
    mir_display_output_type_lvds        = mir_output_type_lvds,
    mir_display_output_type_component   = mir_output_type_component,
    mir_display_output_type_ninepindin  = mir_output_type_ninepindin,
    mir_display_output_type_displayport = mir_output_type_displayport,
    mir_display_output_type_hdmia       = mir_output_type_hdmia,
    mir_display_output_type_hdmib       = mir_output_type_hdmib,
    mir_display_output_type_tv          = mir_output_type_tv,
    mir_display_output_type_edp         = mir_output_type_edp,
    mir_display_output_type_virtual     = mir_output_type_virtual,
    mir_display_output_type_dsi         = mir_output_type_dsi,
    mir_display_output_type_dpi         = mir_output_type_dpi,
} MirDisplayOutputType;

typedef enum MirOutputConnectionState
{
    mir_output_connection_state_disconnected = 0,
    mir_output_connection_state_connected,
    mir_output_connection_state_unknown
} MirOutputConnectionState;

typedef struct MirDisplayMode
{
    uint32_t vertical_resolution;
    uint32_t horizontal_resolution;
    double refresh_rate;
} MirDisplayMode;

enum { mir_display_output_id_invalid = 0 };

typedef struct MirDisplayOutput
{
    uint32_t num_modes;
    MirDisplayMode* modes;
    uint32_t preferred_mode;  /**< There might be no preferred mode, which
                                   is indicated by a value >=num_modes. */
    uint32_t current_mode;

    uint32_t num_output_formats;
    MirPixelFormat* output_formats;
    MirPixelFormat current_format;

    uint32_t card_id;
    uint32_t output_id;
    MirDisplayOutputType type;

    int32_t position_x;
    int32_t position_y;
    uint32_t connected;
    uint32_t used;

    uint32_t physical_width_mm;
    uint32_t physical_height_mm;

    MirPowerMode power_mode;
    MirOrientation orientation;
} MirDisplayOutput;

typedef struct MirDisplayConfiguration
{
    uint32_t num_outputs;
    MirDisplayOutput* outputs;
    uint32_t num_cards;
    MirDisplayCard *cards;
} MirDisplayConfiguration;

/**
 * The displacement from the top-left corner of the surface.
 */
typedef struct MirBufferStreamInfo
{
    MirBufferStream* stream;
    int displacement_x;
    int displacement_y;
} MirBufferStreamInfo;

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
 * MirScreencastParameters is the structure of required information that
 * you must provide to Mir in order to create a MirScreencast.
 * The width and height parameters can be used to down-scale the screencast
 * For no scaling set them to the region width and height.
 */
typedef struct MirScreencastParameters
{
    /**
     * The rectangular region of the screen to capture -
     * The region is specified in virtual screen space hence multiple screens can be captured simultaneously
     */
    MirRectangle region;
    /** The width of the screencast which can be different than the screen region capture width */
    unsigned int width;
    /** The height of the screencast which can be different than the screen region capture height */
    unsigned int height;
    /**
     * The pixel format of the screencast.
     * It must be a supported format obtained from mir_connection_get_available_surface_formats.
     */
    MirPixelFormat pixel_format;
} MirScreencastParameters;

/**
 * Callback to be passed when calling MirScreencast functions.
 *   \param [in] screencast          the screencast being updated
 *   \param [in,out] client_context  context provided by the client
 */
typedef void (*MirScreencastCallback)(
    MirScreencast *screencast, void *client_context);
typedef MirScreencastCallback mir_screencast_callback
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirScreencastCallback instead");

/**
 * Callback member of MirPromptSession for handling of prompt sessions.
 *   \param [in] prompt_provider  The prompt session associated with the callback
 *   \param [in,out] context      The context provided by the client
 */
typedef void (*MirPromptSessionCallback)(
    MirPromptSession* prompt_provider, void* context);
typedef MirPromptSessionCallback mir_prompt_session_callback
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirPromptSessionCallback instead");

/**
 * Callback member of MirPromptSession for handling of prompt sessions events.
 *   \param [in] prompt_provider  The prompt session associated with the callback
 *   \param [in] state            The state of the prompt session
 *   \param [in,out] context      The context provided by the client
 */
typedef void (*MirPromptSessionStateChangeCallback)(
    MirPromptSession* prompt_provider, MirPromptSessionState state,
    void* context);
typedef MirPromptSessionStateChangeCallback
        mir_prompt_session_state_change_callback
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirPromptSessionStateChangeCallback instead");

/**
 * Callback called when a platform operation completes.
 *
 *   \warning The reply is owned by the callee, who should release it when it's
 *            not needed any more.
 *
 *   \param [in] connection   The connection associated with the platform operation
 *   \param [in] reply        The platform operation reply
 *   \param [in,out] context  The context provided by the client
 */
typedef void (*MirPlatformOperationCallback)(
    MirConnection* connection, MirPlatformMessage* reply, void* context);
typedef MirPlatformOperationCallback mir_platform_operation_callback
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirPlatformOperationCallback instead");

/**
 * Callback called when a change of input devices has occurred
 *   \param [in] connection   The connection associated with the input device
 *                            change
 *   \param [in,out] context  The context provided by client
 */

typedef void (*MirInputConfigCallback)(
    MirConnection* connection, void* context);
typedef MirInputConfigCallback mir_input_config_callback
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirInputConfigCallback instead");

typedef void (*MirBufferCallback)(MirBuffer*, void* context);

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

typedef void (*MirErrorCallback)(
    MirConnection* connection, MirError const* error, void* context);
typedef MirErrorCallback mir_error_callback
    MIR_FOR_REMOVAL_IN_VERSION_1("Use MirErrorCallback instead");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

typedef void (*mir_surface_callback)(MirSurface *surface, void *client_context)
MIR_FOR_REMOVAL_IN_VERSION_1("Use MirWindowCallback instead");

typedef void (*mir_surface_event_callback)(
    MirSurface* surface, MirEvent const* event, void* context)
MIR_FOR_REMOVAL_IN_VERSION_1("Use MirWindowEventCallback instead");

typedef void (*mir_surface_id_callback)(
    MirSurface* surface, MirPersistentId* id, void* context)
MIR_FOR_REMOVAL_IN_VERSION_1("Use MirWindowIdCallback instead");

typedef MirSurfaceParameters MirWindowParameters
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_get_xxx apis or listen for attribute events instead");

#pragma GCC diagnostic pop

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_CLIENT_TYPES_H_ */
