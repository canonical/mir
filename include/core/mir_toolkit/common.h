/*
 * Simple definitions common to client and server.
 *
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_COMMON_H_
#define MIR_COMMON_H_

//for clang
#ifndef __has_feature
  #define __has_feature(x) 0  // Compatibility with non-clang
#endif

//for clang
#ifndef __has_extension
  #define __has_extension __has_feature // Compatibility with pre-3.0
#endif

/* This is C code. Not C++. */

/**
 * Attributes of a window that the client and server/shell may wish to
 * get or set over the wire.
 */
typedef enum MirWindowAttrib
{
    /* Do not specify values...code relies on 0...N ordering. */
    mir_window_attrib_type,
    mir_window_attrib_state,
    mir_window_attrib_focus = mir_window_attrib_state+2,
    mir_window_attrib_dpi,
    mir_window_attrib_visibility,
    mir_window_attrib_preferred_orientation,
    /* Must be last */
    mir_window_attribs
} MirWindowAttrib;

typedef enum MirWindowType
{
    mir_window_type_normal,       /**< AKA "regular"                       */
    mir_window_type_utility,      /**< AKA "floating"                      */
    mir_window_type_dialog,
    mir_window_type_gloss,
    mir_window_type_freestyle,
    mir_window_type_menu,
    mir_window_type_inputmethod,  /**< AKA "OSK" or handwriting etc.       */
    mir_window_type_satellite,    /**< AKA "toolbox"/"toolbar"             */
    mir_window_type_tip,          /**< AKA "tooltip"                       */
    mir_window_type_decoration,
    mir_window_types
} MirWindowType;

typedef enum MirWindowState
{
    mir_window_state_unknown,
    mir_window_state_restored,
    mir_window_state_minimized,
    mir_window_state_maximized,
    mir_window_state_vertmaximized,
    /* mir_window_state_semimaximized,
       Omitted for now, since it's functionally a subset of vertmaximized and
       differs only in the X coordinate. */
    mir_window_state_fullscreen,
    mir_window_state_horizmaximized,
    mir_window_state_hidden,
    mir_window_state_attached,       /**< Used for panels, notifications and other windows attached to output edges */
    mir_window_states
} MirWindowState;

typedef enum MirWindowFocusState
{
    mir_window_focus_state_unfocused = 0,   /**< Inactive and does not have focus           */
    mir_window_focus_state_focused,         /**< Active and has keybaord focus              */
    mir_window_focus_state_active           /**< Active but does not have keyboard focus    */
} MirWindowFocusState;

typedef enum MirWindowVisibility
{
    mir_window_visibility_occluded = 0,
    mir_window_visibility_exposed
} MirWindowVisibility;

typedef enum MirLifecycleState
{
    mir_lifecycle_state_will_suspend,
    mir_lifecycle_state_resumed,
    mir_lifecycle_connection_lost
} MirLifecycleState;

typedef enum MirPowerMode
{
    mir_power_mode_on, /* Display in use. */
    mir_power_mode_standby, /* Blanked, low power. */
    mir_power_mode_suspend, /* Blanked, lowest power. */
    mir_power_mode_off /* Powered down. */
} MirPowerMode;

typedef enum MirOutputType
{
    mir_output_type_unknown     = 0,  /* DRM_MODE_CONNECTOR_Unknown     */
    mir_output_type_vga         = 1,  /* DRM_MODE_CONNECTOR_VGA         */
    mir_output_type_dvii        = 2,  /* DRM_MODE_CONNECTOR_DVII        */
    mir_output_type_dvid        = 3,  /* DRM_MODE_CONNECTOR_DVID        */
    mir_output_type_dvia        = 4,  /* DRM_MODE_CONNECTOR_DVIA        */
    mir_output_type_composite   = 5,  /* DRM_MODE_CONNECTOR_Composite   */
    mir_output_type_svideo      = 6,  /* DRM_MODE_CONNECTOR_SVIDEO      */
    mir_output_type_lvds        = 7,  /* DRM_MODE_CONNECTOR_LVDS        */
    mir_output_type_component   = 8,  /* DRM_MODE_CONNECTOR_Component   */
    mir_output_type_ninepindin  = 9,  /* DRM_MODE_CONNECTOR_9PinDIN     */
    mir_output_type_displayport = 10, /* DRM_MODE_CONNECTOR_DisplayPort */
    mir_output_type_hdmia       = 11, /* DRM_MODE_CONNECTOR_HDMIA       */
    mir_output_type_hdmib       = 12, /* DRM_MODE_CONNECTOR_HDMIB       */
    mir_output_type_tv          = 13, /* DRM_MODE_CONNECTOR_TV          */
    mir_output_type_edp         = 14, /* DRM_MODE_CONNECTOR_eDP         */
    mir_output_type_virtual     = 15, /* DRM_MODE_CONNECTOR_VIRTUAL     */
    mir_output_type_dsi         = 16, /* DRM_MODE_CONNECTOR_DSI         */
    mir_output_type_dpi         = 17, /* DRM_MODE_CONNECTOR_DPI         */
} MirOutputType;

typedef enum MirPromptSessionState
{
    mir_prompt_session_state_stopped = 0,
    mir_prompt_session_state_started,
    mir_prompt_session_state_suspended
} MirPromptSessionState;

/**
 * 32-bit pixel formats (8888):
 * The order of components in the enum matches the order of the components
 * as they would be written in an integer representing a pixel value of that
 * format. For example; abgr_8888 should be coded as 0xAABBGGRR, which will
 * end up as R,G,B,A in memory on a little endian system, and as A,B,G,R on a
 * big endian system.
 *
 * 24-bit pixel formats (888):
 * These are in literal byte order, regardless of CPU architecture it's always
 * the same. Writing these 3-byte pixels is typically slower than other formats
 * but uses less memory than 32-bit and is endian-independent.
 *
 * 16-bit pixel formats (565/5551/4444):
 * Always interpreted as one 16-bit integer per pixel with components in
 * high-to-low bit order following the format name. These are the fastest
 * formats, however colour quality is visibly lower.
 */
typedef enum MirPixelFormat
{
    mir_pixel_format_invalid = 0,
    mir_pixel_format_abgr_8888 = 1,
    mir_pixel_format_xbgr_8888 = 2,
    mir_pixel_format_argb_8888 = 3,
    mir_pixel_format_xrgb_8888 = 4,
    mir_pixel_format_bgr_888 = 5,
    mir_pixel_format_rgb_888 = 6,
    mir_pixel_format_rgb_565 = 7,
    mir_pixel_format_rgba_5551 = 8,
    mir_pixel_format_rgba_4444 = 9,
    /*
     * TODO: Big endian support would require additional formats in order to
     *       composite software surfaces using OpenGL (GL_RGBA/GL_BGRA_EXT):
     *         mir_pixel_format_rgb[ax]_8888
     *         mir_pixel_format_bgr[ax]_8888
     */
    mir_pixel_formats   /* Note: This is always max format + 1 */
} MirPixelFormat;

/* This could be improved... https://bugs.launchpad.net/mir/+bug/1236254 */
#define MIR_BYTES_PER_PIXEL(f) ((f) == mir_pixel_format_bgr_888   ? 3 : \
                                (f) == mir_pixel_format_rgb_888   ? 3 : \
                                (f) == mir_pixel_format_rgb_565   ? 2 : \
                                (f) == mir_pixel_format_rgba_5551 ? 2 : \
                                (f) == mir_pixel_format_rgba_4444 ? 2 : \
                                                                    4)

/** Direction relative to the "natural" orientation of the display */
typedef enum MirOrientation
{
    mir_orientation_normal = 0,
    mir_orientation_left = 90,
    mir_orientation_inverted = 180,
    mir_orientation_right = 270
} MirOrientation;

/** Mirroring axis relative to the "natural" orientation of the display */
typedef enum MirMirrorMode
{
    mir_mirror_mode_none,
    mir_mirror_mode_vertical,
    mir_mirror_mode_horizontal
} MirMirrorMode;

typedef enum MirOrientationMode
{
    mir_orientation_mode_portrait = 1 << 0,
    mir_orientation_mode_landscape = 1 << 1,
    mir_orientation_mode_portrait_inverted = 1 << 2,
    mir_orientation_mode_landscape_inverted = 1 << 3,
    mir_orientation_mode_portrait_any = mir_orientation_mode_portrait |
                                        mir_orientation_mode_portrait_inverted,
    mir_orientation_mode_landscape_any = mir_orientation_mode_landscape |
                                         mir_orientation_mode_landscape_inverted,
    mir_orientation_mode_any = mir_orientation_mode_portrait_any |
                               mir_orientation_mode_landscape_any
} MirOrientationMode;

typedef enum MirEdgeAttachment
{
    mir_edge_attachment_vertical = 1 << 0,
    mir_edge_attachment_horizontal = 1 << 1,
    mir_edge_attachment_any = mir_edge_attachment_vertical |
                              mir_edge_attachment_horizontal
} MirEdgeAttachment;

// Inspired by GdkGravity
/**
 * Reference point for aligning a surface relative to a rectangle.
 * Each element (surface and rectangle) has a MirPlacementGravity assigned.
 */
typedef enum MirPlacementGravity
{
    /// the reference point is at the center.
    mir_placement_gravity_center    = 0,

    /// the reference point is at the middle of the left edge.
    mir_placement_gravity_west      = 1 << 0,

    /// the reference point is at the middle of the right edge.
    mir_placement_gravity_east      = 1 << 1,

    /// the reference point is in the middle of the top edge.
    mir_placement_gravity_north     = 1 << 2,

    /// the reference point is at the middle of the lower edge.
    mir_placement_gravity_south     = 1 << 3,

    /// the reference point is at the top left corner.
    mir_placement_gravity_northwest = mir_placement_gravity_north | mir_placement_gravity_west,

    /// the reference point is at the top right corner.
    mir_placement_gravity_northeast = mir_placement_gravity_north | mir_placement_gravity_east,

    /// the reference point is at the lower left corner.
    mir_placement_gravity_southwest = mir_placement_gravity_south | mir_placement_gravity_west,

    /// the reference point is at the lower right corner.
    mir_placement_gravity_southeast = mir_placement_gravity_south | mir_placement_gravity_east
} MirPlacementGravity;

// Inspired by GdkAnchorHints
/**
 * Positioning hints for aligning a window relative to a rectangle.
 *
 * These hints determine how the window should be positioned in the case that
 * the surface would fall off-screen if placed in its ideal position.
 *
 * For example, \p mir_placement_hints_flip_x will invert the x component of
 * \p aux_rect_placement_offset and replace \p mir_placement_gravity_northwest
 * with \p mir_placement_gravity_northeast and vice versa if the window extends
 * beyond the left or right edges of the monitor.
 *
 * If \p mir_placement_hints_slide_x is set, the window can be shifted
 * horizontally to fit on-screen.
 *
 * If \p mir_placement_hints_resize_x is set, the window can be shrunken
 * horizontally to fit.
 *
 * If \p mir_placement_hints_antipodes is set then the rect gravity may be
 * substituted with the opposite corner (e.g. \p mir_placement_gravity_northeast
 * to \p mir_placement_gravity_southwest) in combination with other options.
 *
 * When multiple flags are set, flipping should take precedence over sliding,
 * which should take precedence over resizing.
 */
typedef enum MirPlacementHints
{
    /// allow flipping anchors horizontally
    mir_placement_hints_flip_x   = 1 << 0,

    /// allow flipping anchors vertically
    mir_placement_hints_flip_y   = 1 << 1,

    /// allow sliding window horizontally
    mir_placement_hints_slide_x  = 1 << 2,

    /// allow sliding window vertically
    mir_placement_hints_slide_y  = 1 << 3,

    /// allow resizing window horizontally
    mir_placement_hints_resize_x = 1 << 4,

    /// allow resizing window vertically
    mir_placement_hints_resize_y = 1 << 5,

    /// allow flipping aux_anchor to opposite corner
    mir_placement_hints_antipodes= 1 << 6,

    /// allow flipping anchors on both axes
    mir_placement_hints_flip_any = mir_placement_hints_flip_x|mir_placement_hints_flip_y,

    /// allow sliding window on both axes
    mir_placement_hints_slide_any  = mir_placement_hints_slide_x|mir_placement_hints_slide_y,

    /// allow resizing window on both axes
    mir_placement_hints_resize_any = mir_placement_hints_resize_x|mir_placement_hints_resize_y,
} MirPlacementHints;


/**
 * Hints for resizing a window.
 *
 * These values are used to indicate which edge(s) of a surface
 * is being dragged in a resize operation.
 */
typedef enum MirResizeEdge
{
    mir_resize_edge_none      = 0,
    mir_resize_edge_west      = 1 << 0,
    mir_resize_edge_east      = 1 << 1,
    mir_resize_edge_north     = 1 << 2,
    mir_resize_edge_south     = 1 << 3,
    mir_resize_edge_northwest = mir_resize_edge_north | mir_resize_edge_west,
    mir_resize_edge_northeast = mir_resize_edge_north | mir_resize_edge_east,
    mir_resize_edge_southwest = mir_resize_edge_south | mir_resize_edge_west,
    mir_resize_edge_southeast = mir_resize_edge_south | mir_resize_edge_east
} MirResizeEdge;

/**
 * Form factor associated with a physical output
 */
typedef enum MirFormFactor
{
    mir_form_factor_unknown,
    mir_form_factor_phone,
    mir_form_factor_tablet,
    mir_form_factor_monitor,
    mir_form_factor_tv,
    mir_form_factor_projector,
} MirFormFactor;


/**
 * Physical arrangement of subpixels on the physical output
 *
 * This is always relative to the “natural” orientation of the display - mir_orientation_normal.
 */
typedef enum MirSubpixelArrangement
{
    mir_subpixel_arrangement_unknown,           /**< Arrangement of subpixels cannot be determined */
    mir_subpixel_arrangement_horizontal_rgb,    /**< Subpixels are arranged horizontally, R, G, B from left to right */
    mir_subpixel_arrangement_horizontal_bgr,    /**< Subpixels are arranged horizontally, B, G, R from left to right */
    mir_subpixel_arrangement_vertical_rgb,      /**< Subpixels are arranged vertically, R, G, B from top to bottom */
    mir_subpixel_arrangement_vertical_bgr,      /**< Subpixels are arranged vertically, B, G, R from top to bottom */
    mir_subpixel_arrangement_none               /**< Device does not have regular subpixels */
} MirSubpixelArrangement;

/**
 * Shell chrome
 */
typedef enum MirShellChrome
{
    mir_shell_chrome_normal,
    mir_shell_chrome_low,
} MirShellChrome;

/**
 * Pointer Confinement
 */
typedef enum MirPointerConfinementState
{
    mir_pointer_unconfined,
    mir_pointer_confined_oneshot,
    mir_pointer_confined_persistent,
    mir_pointer_locked_oneshot,
    mir_pointer_locked_persistent,
} MirPointerConfinementState;

/**
 * Supports gamma correction
 */
typedef enum MirOutputGammaSupported
{
    mir_output_gamma_unsupported,
    mir_output_gamma_supported
} MirOutputGammaSupported;

/**
 * Depth layer controls Z ordering of surfaces.
 *
 * A surface will always appear on top of surfaces with a lower depth layer, and below those with a higher one.
 * A depth layer can be converted to a number with mir::mir_depth_layer_get_index().
 * This is useful for creating a list indexed by depth layer, or comparing the height of two layers.
 */
typedef enum MirDepthLayer
{
    mir_depth_layer_background,         /**< For desktop backgrounds and alike (lowest layer) */
    mir_depth_layer_below,              /**< For panels or other controls/decorations below normal windows */
    mir_depth_layer_application,        /**< For normal application windows */
    mir_depth_layer_always_on_top,      /**< For always-on-top application windows */
    mir_depth_layer_above,              /**< For panels or notifications that want to be above normal windows */
    mir_depth_layer_overlay,            /**< For overlays such as lock screens (heighest layer) */
} MirDepthLayer;

/**
 * Focus mode controls how a surface gains and loses focus.
 */
typedef enum MirFocusMode
{
    mir_focus_mode_focusable,    /**< The surface can gain and lose focus normally */
    mir_focus_mode_disabled,     /**< The surface will never be given focus */
    mir_focus_mode_grabbing,     /**< This mode causes the surface to take focus if possible, and prevents focus from
                                      leaving it as long as it has this mode */
} MirFocusMode;

#endif
