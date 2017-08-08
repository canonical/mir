/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

// MirSurfaceOutputEvent is a deprecated type, but we need to implement it
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

MirSurfaceOutputEvent::MirSurfaceOutputEvent()
{
    event.initSurfaceOutput();
}

int MirSurfaceOutputEvent::surface_id() const
{
    return event.asReader().getSurfaceOutput().getSurfaceId();
}

void MirSurfaceOutputEvent::set_surface_id(int id)
{
    event.getSurfaceOutput().setSurfaceId(id);
}

int MirSurfaceOutputEvent::dpi() const
{
    return event.asReader().getSurfaceOutput().getDpi();
}

void MirSurfaceOutputEvent::set_dpi(int dpi)
{
    event.getSurfaceOutput().setDpi(dpi);
}

float MirSurfaceOutputEvent::scale() const
{
    return event.asReader().getSurfaceOutput().getScale();
}

void MirSurfaceOutputEvent::set_scale(float scale)
{
    event.getSurfaceOutput().setScale(scale);
}

double MirSurfaceOutputEvent::refresh_rate() const
{
    return event.asReader().getSurfaceOutput().getRefreshRate();
}

void MirSurfaceOutputEvent::set_refresh_rate(double rate)
{
    event.getSurfaceOutput().setRefreshRate(rate);
}

MirFormFactor MirSurfaceOutputEvent::form_factor() const
{
    return static_cast<MirFormFactor>(event.asReader().getSurfaceOutput().getFormFactor());
}

void MirSurfaceOutputEvent::set_form_factor(MirFormFactor factor)
{
    event.getSurfaceOutput().setFormFactor(static_cast<mir::capnp::SurfaceOutputEvent::FormFactor>(factor));
}

uint32_t MirSurfaceOutputEvent::output_id() const
{
    return event.asReader().getSurfaceOutput().getOutputId();
}

void MirSurfaceOutputEvent::set_output_id(uint32_t id)
{
    event.getSurfaceOutput().setOutputId(id);
}
