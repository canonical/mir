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

#include "drag_and_drop.h"
#include "mir_toolkit/extensions/drag_and_drop.h"

#include "mir/uncaught.h"
#include "mir/events/surface_event.h"
#include "mir/events/pointer_event.h"

#include "mir_surface.h"

namespace
{

void request_drag_and_drop(MirWindow* window, MirCookie const* cookie)
try
{
    window->request_drag_and_drop(cookie);
}
catch (std::exception const& e)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(e);
    abort();
}

void set_start_drag_and_drop_callback(MirWindow* window,
    void (*callback)(MirWindow* window, MirDragAndDropEvent const* event, void* context),
    void* context)
try
{
    window->set_drag_and_drop_start_handler([callback,window,context](MirWindowEvent const* event)
        { callback(window, reinterpret_cast<MirDragAndDropEvent const*>(event), context); });
}
catch (std::exception const& e)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(e);
    abort();
}

MirBlob* start_drag_and_drop(MirDragAndDropEvent const* event)
try
{
    return reinterpret_cast<MirWindowEvent const*>(event)->dnd_handle();
}
catch (std::exception const& e)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(e);
    abort();
}

MirBlob* pointer_drag_and_drop(MirPointerEvent const* event)
try
{
    return event->dnd_handle();
}
catch (std::exception const& e)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(e);
    abort();
}

MirDragAndDropV1 const impl{
    &request_drag_and_drop,
    &set_start_drag_and_drop_callback,
    &start_drag_and_drop,
    &pointer_drag_and_drop};
}

MirDragAndDropV1 const* const mir::drag_and_drop::v1 = &impl;
