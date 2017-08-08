/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_DRAG_AND_DROP_H
#define MIR_DRAG_AND_DROP_H

#include "mir_toolkit/mir_extension_core.h"
#include "mir_toolkit/client_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MirDragAndDropEvent MirDragAndDropEvent;

typedef struct MirDragAndDropV1
{
    /**
     * Request drag and drop. If the request succeeds a "start drag" handle will
     * be received via the callback set by set_start_drag_and_drop_callback().
     *
     * \warning An invalid cookie will terminate the client connection.
     *
     * \param [in] window  The source window
     * \param [in] cookie  A cookie instance obtained from an input event.
     */
    void (*request_drag_and_drop)(MirWindow* window, MirCookie const* cookie);

    /**
     * Set the drag and drop callback.
     * This receives a MirDragAndDropEvent if and when drag and drop begins.
     *
     * \param [in] window   The window
     * \param [in] callback The callback function.
     * \param [in] context  To be passed to callback
     */
    void (*set_start_drag_and_drop_callback)(MirWindow* window,
               void (*callback)(MirWindow* window, MirDragAndDropEvent const* event, void* context),
               void* context);

    /**
     * Retrieve any "start drag & drop" handle associated with the event.
     * The handle is used to provide drag and drop metadata and content to the
     * service supporting drag and drop (e.g. on Ubuntu the content hub).
     *
     * \param [in] event The event
     * \return           The associated drag handle or NULL
     */
    MirBlob* (*start_drag_and_drop)(MirDragAndDropEvent const* event);

    /**
     * Retrieve any "drag & drop" handle associated with the event.
     * The handle is used to retrieve drag and drop metadata and content from
     * the service supporting drag and drop (e.g. on Ubuntu the content hub).
     *
     * \param [in] event The event
     * \return           The associated drag handle or NULL
     */
    MirBlob* (*pointer_drag_and_drop)(MirPointerEvent const* event);

} MirDragAndDropV1;

static inline MirDragAndDropV1 const* mir_drag_and_drop_v1(MirConnection* connection)
{
    return (MirDragAndDropV1 const*) mir_connection_request_extension(connection, "mir_drag_and_drop", 1);
}

#ifdef __cplusplus
}
#endif
#endif //MIR_DRAG_AND_DROP_H