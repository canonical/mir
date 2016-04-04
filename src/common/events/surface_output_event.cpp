/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/events/surface_output_event.h"

MirSurfaceOutputEvent::MirSurfaceOutputEvent() :
    MirEvent(mir_event_type_surface_output)
{
}

int MirSurfaceOutputEvent::surface_id() const
{
    return surface_id_;
}

void MirSurfaceOutputEvent::set_surface_id(int id)
{
    surface_id_ = id;
}

int MirSurfaceOutputEvent::dpi() const
{
    return dpi_;
}

void MirSurfaceOutputEvent::set_dpi(int dpi)
{
    dpi_ = dpi;
}

float MirSurfaceOutputEvent::scale() const
{
    return scale_;
}

void MirSurfaceOutputEvent::set_scale(float scale)
{
    scale_ = scale;
}

MirFormFactor MirSurfaceOutputEvent::form_factor() const
{
    return form_factor_;
}

void MirSurfaceOutputEvent::set_form_factor(MirFormFactor factor)
{
    form_factor_ = factor;
}

uint32_t MirSurfaceOutputEvent::output_id() const
{
    return output_id_;
}

void MirSurfaceOutputEvent::set_output_id(uint32_t id)
{
    output_id_ = id;
}
