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


namespace mi = mir::input;

mi::PointerConfiguration::PointerConfiguration() :
    primary_button_{mir_pointer_button_primary}, cursor_speed_{0.0}, horizontal_scroll_speed_{1.0}, vertical_scroll_speed_{1.0}
{
}

mi::PointerConfiguration::PointerConfiguration(MirPointerButton primary, double cursor_speed,
                                               double horizontal_scroll_speed, double vertical_scroll_speed) :
    primary_button_{primary}, cursor_speed_{cursor_speed}, horizontal_scroll_speed_{horizontal_scroll_speed},
    vertical_scroll_speed_{vertical_scroll_speed}
{
}

mi::PointerConfiguration::~PointerConfiguration() = default;

void mi::PointerConfiguration::primary_button(MirPointerButton button)
{
    primary_button_ = button;
}
MirPointerButton mi::PointerConfiguration::primary_button() const
{
    return primary_button_;
}

void mi::PointerConfiguration::cursor_speed(double speed)
{
    cursor_speed_ = speed;
}

double mi::PointerConfiguration::cursor_speed() const
{
    return cursor_speed_;
}

void mi::PointerConfiguration::horizontal_scroll_speed(double horizontal)
{
    horizontal_scroll_speed_ = horizontal;
}

double mi::PointerConfiguration::horizontal_scroll_speed() const
{
    return horizontal_scroll_speed_;
}

void mi::PointerConfiguration::vertical_scroll_speed(double vertical)
{
    vertical_scroll_speed_ = vertical;
}

double mi::PointerConfiguration::vertical_scroll_speed() const
{
    return vertical_scroll_speed_;
}
