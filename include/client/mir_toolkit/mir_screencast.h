/*
 * Copyright © 2014 Canonical Ltd.
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
 */

#ifndef MIR_TOOLKIT_MIR_SCREENCAST_H_
#define MIR_TOOLKIT_MIR_SCREENCAST_H_

#include <mir_toolkit/client_types.h>
#include <mir_toolkit/deprecations.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

typedef enum MirScreencastResult
{
    /**
     * Screencasting to the MirBuffer succeeded.
     */
    mir_screencast_success,

    /**
     * Screencasting failed.
     */
    mir_screencast_error_failure,
} MirScreencastResult;

typedef void (*MirScreencastBufferCallback)(
    MirScreencastResult status, MirBuffer* buffer, void* context);

/**
 * Create a screencast specification.
 *
 * \remark For use with mir_screencast_create() at the width, height,
 * pixel format and capture region must be set.
 *
 * \param [in] connection   a valid mir connection
 * \return                  A handle that can ultimately be passed to
 *                          mir_create_window() or mir_window_apply_spec()
 */
MirScreencastSpec* mir_create_screencast_spec(MirConnection* connection);

/**
 * Set the requested width, in pixels
 *
 * \param [in] spec     Specification to mutate
 * \param [in] width    Requested width.
 *
 */
void mir_screencast_spec_set_width(MirScreencastSpec* spec, unsigned int width);

/**
 * Set the requested height, in pixels
 *
 * \param [in] spec     Specification to mutate
 * \param [in] height   Requested height.
 *
 */
void mir_screencast_spec_set_height(MirScreencastSpec* spec, unsigned int height);

/**
 * Set the requested pixel format.
 *
 * \param [in] spec     Specification to mutate
 * \param [in] format   Requested pixel format
 *
 */
void mir_screencast_spec_set_pixel_format(MirScreencastSpec* spec, MirPixelFormat format);


/**
 * Set the rectangular region to capture.
 *
 * \param [in] spec     Specification to mutate
 * \param [in] region   The rectangular region of the screen to capture
 *                      specified in virtual screen space coordinates
 *
 */
void mir_screencast_spec_set_capture_region(MirScreencastSpec* spec, MirRectangle const* region);

/**
 * Set the requested mirror mode.
 *
 * \param [in] spec     Specification to mutate
 * \param [in] mode     The mirroring mode to apply when screencasting
 *
 */
void mir_screencast_spec_set_mirror_mode(MirScreencastSpec* spec, MirMirrorMode mode);

/**
 * Set the requested number of buffers to use.
 *
 * \param [in] spec     Specification to mutate
 * \param [in] nbuffers The number of buffers to allocate for screencasting
 *
 */
void mir_screencast_spec_set_number_of_buffers(MirScreencastSpec* spec, unsigned int nbuffers);

/**
 * Release the resources held by a MirScreencastSpec.
 *
 * \param [in] spec     Specification to release
 */
void mir_screencast_spec_release(MirScreencastSpec* spec);

/**
 * Create a screencast from a given specification
 *
 * \param [in] spec  Specification of the screencast attributes
 * \return           The resulting screencast
 */
MirScreencast* mir_screencast_create_sync(MirScreencastSpec* spec);

/**
 * Test for a valid screencast
 *
 * \param [in] screencast  The screencast to verify
 * \return                 True if the supplied screencast is valid, false otherwise.
 */
bool mir_screencast_is_valid(MirScreencast *screencast);

/**
 * Retrieve a text description of the error. The returned string is owned by
 * the library and remains valid until the screencast or the associated
 * connection has been released.
 *
 * \param [in] screencast  The screencast
 * \return                 A text description of any error resulting in an
 *                         invalid screencast, or the empty string "" if the
 *                         screencast is valid.
 */
char const *mir_screencast_get_error_message(MirScreencast *screencast);

/**
 * Create a screencast on the supplied connection.
 *
 * A screencast allows clients to read the contents of the screen.
 *
 *   \warning This request may be denied.
 *   \param [in] connection  The connection
 *   \param [in] parameters  The screencast parameters
 *   \return                 The resulting screencast
 */
MirScreencast* mir_connection_create_screencast_sync(
    MirConnection* connection,
    MirScreencastParameters* parameters)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_screencast_create_sync instead");

/**
 * Release the specified screencast.
 *   \param [in] screencast  The screencast to be released
 */
void mir_screencast_release_sync(
    MirScreencast* screencast);

/**
 * Retrieve the MirBufferStream associated with a screencast 
 * (to advance buffers, obtain EGLNativeWindowType, etc...)
 * 
 *   \param[in] screencast The screencast
 */
MirBufferStream* mir_screencast_get_buffer_stream(MirScreencast* screencast);

/** Capture the contents of the screen to a particular buffer.
 *
 *   \param [in] screencast         The screencast
 *   \param [in] buffer             The buffer
 *   \param [in] available_callback Callback triggered when buffer is available again
 *   \param [in] available_context  The context for the above callback
 **/
void mir_screencast_capture_to_buffer(
    MirScreencast* screencast,
    MirBuffer* buffer,
    MirScreencastBufferCallback available_callback, void* available_context);

/** Capture the contents of the screen to a particular buffer and wait for the
 *  capture to complete.
 *
 *   \warning   The returned MirError will be valid until the next call
 *              to mir_screencast_capture_to_buffer or mir_screencast_capture_to_buffer_sync.
 *   \param [in] screencast         The screencast
 *   \param [in] buffer             The buffer
 *   \return                        The error condition
 **/
MirScreencastResult mir_screencast_capture_to_buffer_sync(MirScreencast* screencast, MirBuffer* buffer);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_SCREENCAST_H_ */
