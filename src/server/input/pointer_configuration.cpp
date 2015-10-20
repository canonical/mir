/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/input/pointer_configuration.h"
#include "mir_toolkit/mir_input_device.h"


namespace mi = mir::input;

mi::PointerConfiguration::PointerConfiguration() :
    handedness_{mir_pointer_handedness_right}, acceleration_bias{0.0}, horizontal_scroll_scale_{1.0}, vertical_scroll_scale_{1.0}
{
}

mi::PointerConfiguration::PointerConfiguration(MirPointerHandedness handedness, double acceleration_bias,
                                               double horizontal_scroll_scale, double vertical_scroll_scale)
    : handedness_{handedness}, acceleration_bias{acceleration_bias}, horizontal_scroll_scale_{horizontal_scroll_scale},
      vertical_scroll_scale_{vertical_scroll_scale}
{
}

mi::PointerConfiguration::~PointerConfiguration() = default;

void mi::PointerConfiguration::handedness(MirPointerHandedness handedness)
{
    handedness_ = handedness;
}

MirPointerHandedness mi::PointerConfiguration::handedness() const
{
    return handedness_;
}

void mi::PointerConfiguration::cursor_acceleration_bias(double bias)
{
    acceleration_bias = bias;
}

double mi::PointerConfiguration::cursor_acceleration_bias() const
{
    return acceleration_bias;
}

void mi::PointerConfiguration::horizontal_scroll_scale(double horizontal)
{
    horizontal_scroll_scale_ = horizontal;
}

double mi::PointerConfiguration::horizontal_scroll_scale() const
{
    return horizontal_scroll_scale_;
}

void mi::PointerConfiguration::vertical_scroll_scale(double vertical)
{
    vertical_scroll_scale_ = vertical;
}

double mi::PointerConfiguration::vertical_scroll_scale() const
{
    return vertical_scroll_scale_;
}
