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

#include "mir/graphics/gamma_curves.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;

mg::GammaCurves::GammaCurves(GammaCurve const& red,
                             GammaCurve const& green,
                             GammaCurve const& blue) :
    red(red),
    green(green),
    blue(blue)
{
    if (red.size() != green.size() ||
        green.size() != blue.size())
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Different gamma LUT sizes"));
    }
}
