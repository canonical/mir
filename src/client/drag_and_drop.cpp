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

namespace
{

void request_drag_and_drop(MirWindow* /*window*/, MirCookie const* /*cookie*/)
{
    // TODO
}

MirBlob* start_drag(MirWindowEvent const* /*event*/)
{
    // TODO
    return nullptr;
}

MirBlob* pointer_dnd_handle(MirPointerEvent const* /*event*/)
{
    // TODO
    return nullptr;
}

MirDragAndDropV1 impl{&request_drag_and_drop, &start_drag, &pointer_dnd_handle};
}

MirDragAndDropV1 const* const mir::drag_and_drop::v1 = &impl;
