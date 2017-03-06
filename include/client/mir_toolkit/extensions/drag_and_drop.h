/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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

typedef struct MirDragAndDropV1
{
    /**
     * Begin drag and drop.
     *
     * \param [in] window  The source window
     * \param [in] cookie  A cookie instance obtained from an input event.
     *                     An invalid cookie will terminate the client connection.
     */
    void (*begin_drag_and_drop)(MirWindow* window, MirCookie const* cookie);

    /**
     * Retrieve any start drag handle associated with the event
     *
     * \param [in] event The event
     * \return           The associated drag handle or NULL
     */
    MirBlob* (*start_drag_handle)(MirWindowEvent const* event);

} MirDragAndDropV1;

static inline MirDragAndDropV1 const* mir_drag_and_drop_v1(
    MirConnection* connection)
{
    return (MirDragAndDropV1 const*) mir_connection_request_extension(connection, "mir_drag_and_drop", 1);
}

#ifdef __cplusplus
}
#endif
#endif //MIR_DRAG_AND_DROP_H
