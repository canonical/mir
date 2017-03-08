/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "drag_and_drop.h"

#include "mir_toolkit/extensions/drag_and_drop.h"
#include "mir_surface.h"

namespace
{

void request_drag_and_drop(MirWindow* window, MirCookie const* cookie)
{
    window->request_drag_and_drop(cookie);
}

MirBlob* start_drag_and_drop(MirWindowEvent const* /*event*/)
{
    // TODO
    return nullptr;
}

MirBlob* pointer_drag_and_drop(MirPointerEvent const* /*event*/)
{
    // TODO
    return nullptr;
}

MirDragAndDropV1 const impl{&request_drag_and_drop, &start_drag_and_drop, &pointer_drag_and_drop};
}

MirDragAndDropV1 const* const mir::drag_and_drop::v1 = &impl;
