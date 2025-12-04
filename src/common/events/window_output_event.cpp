/*
 * Copyright © Canonical Ltd.
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
 */

#include <mir/events/window_output_event.h>

MirWindowOutputEvent::MirWindowOutputEvent() : MirEvent(mir_event_type_window_output)
{
}

auto MirWindowOutputEvent::clone() const -> MirWindowOutputEvent*
{
    return new MirWindowOutputEvent{*this};
}

int MirWindowOutputEvent::surface_id() const
{
    return surface_id_;
}

void MirWindowOutputEvent::set_surface_id(int id)
{
    surface_id_ = id;
}

int MirWindowOutputEvent::dpi() const
{
    return dpi_;
}

void MirWindowOutputEvent::set_dpi(int dpi)
{
    dpi_ = dpi;
}

float MirWindowOutputEvent::scale() const
{
    return scale_;
}

void MirWindowOutputEvent::set_scale(float scale)
{
    scale_ = scale;
}

double MirWindowOutputEvent::refresh_rate() const
{
    return refresh_rate_;
}

void MirWindowOutputEvent::set_refresh_rate(double rate)
{
    refresh_rate_ = rate;
}

MirFormFactor MirWindowOutputEvent::form_factor() const
{
    return form_factor_;
}

void MirWindowOutputEvent::set_form_factor(MirFormFactor factor)
{
    form_factor_ = factor;
}

uint32_t MirWindowOutputEvent::output_id() const
{
    return output_id_;
}

void MirWindowOutputEvent::set_output_id(uint32_t id)
{
    output_id_ = id;
}
