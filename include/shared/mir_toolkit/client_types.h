/*
 * client_types.h: Type definitions used in client apps and libmirclient.
 *
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_TOOLKIT_CLIENT_TYPES_H_
#define MIR_TOOLKIT_CLIENT_TYPES_H_

#include <mir_toolkit/event.h>

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
typedef struct MirSurface MirSurface;

/**
 * Returned by asynchronous functions. Must not be free'd by
 * callers. See the individual function documentation for information
 * on the lifetime of wait handles.
 */
typedef struct MirWaitHandle MirWaitHandle;

/**
 * Callback to be passed when issuing a mir_connect request.
 *   \param [in] connection          the new connection
 *   \param [in,out] client_context  context provided by client in calling
 *                                   mir_connect
 */
typedef void (*mir_connected_callback)(MirConnection *connection, void *client_context);

/**
 * Callback to be passed when calling:
 *  - mir_connection_create_surface
 *  - mir_surface_swap_buffers
 *  - mir_surface_release
 *   \param [in] surface             the surface being updated
 *   \param [in,out] client_context  context provided by client in calling
 *                                   mir_connect
 */
typedef void (*mir_surface_callback)(MirSurface *surface, void *client_context);

/**
 * Callback member of MirEventDelegate for handling of events.
 *   \param [in] surface     The surface on which an event has occurred
 *   \param [in] event       The event to be handled
 *   \param [in,out] context The context provided by client during delegate
 *                           registration.
 */
typedef void (*mir_event_delegate_callback)(
    MirSurface* surface, MirEvent const* event, void* context);

/**
 * Callback called when a display config change has occurred
 *   \param [in] connection     The connection associated with the display change
 *   \param [in,out] context    The context provided by client
 */

typedef void (*mir_display_config_callback)(
    MirConnection* connection, void* callback);

/**
 * The order of components in a format enum matches the
 * order of the components as they would be written in an
 *  integer representing a pixel value of that format.
 *
 * For example, abgr_8888 corresponds to 0xAABBGGRR, which will
 * end up as R,G,B,A in memory in a little endian system, and
 * as A,B,G,R in memory in a big endian system.
 */
typedef enum MirPixelFormat
{
    mir_pixel_format_invalid,
    mir_pixel_format_abgr_8888,
    mir_pixel_format_xbgr_8888,
    mir_pixel_format_argb_8888,
    mir_pixel_format_xrgb_8888,
    mir_pixel_format_bgr_888
} MirPixelFormat;

/**
 * MirBufferUsage specifies how a surface can and will be used. A "hardware"
 * surface can be used for OpenGL accelerated rendering. A "software" surface
 * is one that can be addressed in main memory and blitted to directly.
 */
typedef enum MirBufferUsage
{
    mir_buffer_usage_hardware = 1,
    mir_buffer_usage_software
} MirBufferUsage;

/**
 * MirSurfaceParameters is the structure of minimum required information that
 * you must provide to Mir in order to create a surface.
 */
typedef struct MirSurfaceParameters
{
    char const *name;
    int width;
    int height;
    MirPixelFormat pixel_format;
    MirBufferUsage buffer_usage;
    /**
     * The id of the output to place the surface in.
     *
     * Use one of the output ids from MirDisplayConfiguration/MirDisplayOutput
     * to place a surface on that output. Only fullscreen placements are
     * currently supported. If you don't have special placement requirements,
     * use the value mir_display_output_invalid.
     */
    uint32_t output_id;
} MirSurfaceParameters;

enum { mir_platform_package_max = 32 };

/**
 * The native buffer type for the system the client is connected on
 */
typedef enum MirPlatformType
{
    mir_platform_type_gbm,
    mir_platform_type_android    
} MirPlatformType;

typedef struct MirPlatformPackage
{
    int data_items;
    int fd_items;

    int data[mir_platform_package_max];
    int fd[mir_platform_package_max];
} MirPlatformPackage;


/**
 * Retrieved information about a MirSurface. This is most useful for learning
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
    mir_display_output_type_unknown,
    mir_display_output_type_vga,
    mir_display_output_type_dvii,
    mir_display_output_type_dvid,
    mir_display_output_type_dvia,
    mir_display_output_type_composite,
    mir_display_output_type_svideo,
    mir_display_output_type_lvds,
    mir_display_output_type_component,
    mir_display_output_type_ninepindin,
    mir_display_output_type_displayport,
    mir_display_output_type_hdmia,
    mir_display_output_type_hdmib,
    mir_display_output_type_tv,
    mir_display_output_type_edp
} MirDisplayOutputType;

typedef struct MirDisplayMode
{
    uint32_t vertical_resolution;
    uint32_t horizontal_resolution;
    double refresh_rate;
}MirDisplayMode;

enum { mir_display_output_id_invalid = 0 };

typedef struct MirDisplayOutput
{
    uint32_t num_modes;
    MirDisplayMode* modes;
    uint32_t preferred_mode;
    uint32_t current_mode; 

    uint32_t num_output_formats;
    MirPixelFormat* output_formats;
    uint32_t current_output_format;

    uint32_t card_id;
    uint32_t output_id;
    MirDisplayOutputType type;

    int32_t position_x;
    int32_t position_y;
    uint32_t connected;
    uint32_t used;

    uint32_t physical_width_mm;
    uint32_t physical_height_mm;
} MirDisplayOutput;

typedef struct MirDisplayConfiguration
{
    uint32_t num_outputs;
    MirDisplayOutput* outputs;
    uint32_t num_cards;
    MirDisplayCard *cards;
} MirDisplayConfiguration;

/**
 * MirEventDelegate may be used to specify (at surface creation time)
 * callbacks for handling of events.
 */
typedef struct MirEventDelegate
{
    mir_event_delegate_callback callback;
    void *context;
} MirEventDelegate;

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_CLIENT_TYPES_H_ */
