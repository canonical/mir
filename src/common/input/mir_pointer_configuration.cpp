/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/input/mir_pointer_configuration.h"
#include <ostream>

struct MirPointerConfiguration::Implementation
{
    MirPointerHandedness handedness{mir_pointer_handedness_right};
    MirPointerAcceleration acceleration{mir_pointer_acceleration_adaptive};
    double cursor_acceleration_bias{0.0};
    double horizontal_scroll_scale{1.0};
    double vertical_scroll_scale{1.0};

    Implementation() = default;
    Implementation(MirPointerHandedness handedness,
                   MirPointerAcceleration acceleration,
                   double cursor_acceleration_bias,
                   double horizontal_scroll_scale,
                   double vertical_scroll_scale)
        : handedness{handedness},
          acceleration{acceleration},
          cursor_acceleration_bias{cursor_acceleration_bias},
          horizontal_scroll_scale{horizontal_scroll_scale},
          vertical_scroll_scale{vertical_scroll_scale}
    {
    }
};

MirPointerConfiguration::MirPointerConfiguration()
    : impl{std::make_unique<Implementation>()}
{
}

MirPointerConfiguration::~MirPointerConfiguration() = default;

MirPointerConfiguration::MirPointerConfiguration(MirPointerConfiguration const& cp)
    : impl{std::make_unique<Implementation>(*cp.impl)}
{
}

MirPointerConfiguration::MirPointerConfiguration(MirPointerConfiguration && cp)
    : impl{std::move(cp.impl)}
{
}

MirPointerConfiguration& MirPointerConfiguration::operator=(MirPointerConfiguration const& cp)
{
    *impl = *cp.impl;
    return *this;
}

MirPointerConfiguration::MirPointerConfiguration(MirPointerHandedness handedness,
                                               MirPointerAcceleration acceleration,
                                               double acceleration_bias,
                                               double horizontal_scroll_scale,
                                               double vertical_scroll_scale)
    : impl{std::make_unique<Implementation>(
          handedness, acceleration, acceleration_bias, horizontal_scroll_scale, vertical_scroll_scale)}
{
}

MirPointerHandedness MirPointerConfiguration::handedness() const
{
    return impl->handedness;
}

void MirPointerConfiguration::handedness(MirPointerHandedness value)
{
    impl->handedness = value;
}

MirPointerAcceleration MirPointerConfiguration::acceleration() const
{
    return impl->acceleration;
}

void MirPointerConfiguration::acceleration(MirPointerAcceleration acceleration)
{
    impl->acceleration = acceleration;
}

double MirPointerConfiguration::cursor_acceleration_bias() const
{
    return impl->cursor_acceleration_bias;
}

void MirPointerConfiguration::cursor_acceleration_bias(double bias)
{
    impl->cursor_acceleration_bias = bias;
}

double MirPointerConfiguration::horizontal_scroll_scale() const
{
    return impl->horizontal_scroll_scale;
}

void MirPointerConfiguration::horizontal_scroll_scale(double scale)
{
    impl->horizontal_scroll_scale = scale;
}

double MirPointerConfiguration::vertical_scroll_scale() const
{
    return impl->vertical_scroll_scale;
}

void MirPointerConfiguration::vertical_scroll_scale(double scale)
{
    impl->vertical_scroll_scale = scale;
}


bool MirPointerConfiguration::operator==(MirPointerConfiguration const& rhs) const
{
    return handedness() == rhs.handedness() && acceleration() == rhs.acceleration() &&
           cursor_acceleration_bias() == rhs.cursor_acceleration_bias() &&
           horizontal_scroll_scale() == rhs.horizontal_scroll_scale() &&
           vertical_scroll_scale() == rhs.vertical_scroll_scale();
}

bool MirPointerConfiguration::operator!=(MirPointerConfiguration const& rhs) const
{
    return !(*this == rhs);
}

std::ostream& operator<<(std::ostream& out, MirPointerConfiguration const& rhs)
{
    return out << " handedness:" << rhs.handedness()
        << " acceleration:" << rhs.acceleration()
        << " acceleration_bias:" << rhs.cursor_acceleration_bias()
        << " horizontal_scroll_scale:" << rhs.horizontal_scroll_scale()
        << " vertical_scroll_scale:" << rhs.vertical_scroll_scale();
}
