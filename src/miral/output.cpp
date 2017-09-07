/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
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

#include "miral/output.h"

#include <mir/graphics/display_configuration.h>

miral::Output::Output(const mir::graphics::DisplayConfigurationOutput& output) :
    self{std::make_shared<mir::graphics::DisplayConfigurationOutput>(output)}
{
}

miral::Output::Output(Output const&) = default;
miral::Output& miral::Output::operator=(Output const&) = default;
miral::Output::~Output() = default;

auto miral::Output::type() const -> Type
{
    return Type(self->type);
}

auto miral::Output::physical_size_mm() const -> PhysicalSizeMM
{
    auto const& size = self->physical_size_mm;
    return PhysicalSizeMM{size.width.as_int(), size.height.as_int()};
}

auto miral::Output::connected() const -> bool
{
    return self->connected;
}

auto miral::Output::used() const -> bool
{
    return self->used;
}

auto miral::Output::pixel_format() const -> MirPixelFormat
{
    return self->current_format;
}

auto miral::Output::refresh_rate() const -> double
{
    return self->modes[self->current_mode_index].vrefresh_hz;
}

auto miral::Output::power_mode() const -> MirPowerMode
{
    return self->power_mode;
}

auto miral::Output::orientation() const -> MirOrientation
{
    return self->orientation;
}

auto miral::Output::scale() const -> float
{
    return self->scale;
}

auto miral::Output::form_factor() const -> MirFormFactor
{
    return self->form_factor;
}

auto miral::Output::extents() const -> Rectangle
{
    return self->extents();
}

auto miral::Output::valid() const -> bool
{
    return self->valid();
}

auto miral::Output::is_same_output(Output const& other) const -> bool
{
    return self->card_id == other.self->card_id && self->id == other.self->id;
}

bool miral::operator==(Output::PhysicalSizeMM const& lhs, Output::PhysicalSizeMM const& rhs)
{
    return lhs.width == rhs.width && lhs.height == rhs.height;
}

auto miral::equivalent_display_area(Output const& lhs, Output const& rhs) -> bool
{
    // Eliminate the cases where one or both output isn't available
    auto const lhs_bad = !lhs.valid() || !lhs.used() || !lhs.connected() || lhs.power_mode() != mir_power_mode_on;
    auto const rhs_bad = !rhs.valid() || !rhs.used() || !rhs.connected() || rhs.power_mode() != mir_power_mode_on;
    if (lhs_bad || rhs_bad) return lhs_bad == rhs_bad;

    return lhs.extents() == rhs.extents() &&
           lhs.form_factor() == rhs.form_factor() &&
        lhs.orientation() == rhs.orientation() &&
           lhs.pixel_format() == rhs.pixel_format() &&
        lhs.physical_size_mm() == rhs.physical_size_mm() &&
           lhs.scale() == rhs.scale() &&
           lhs.type() == rhs.type();
}
