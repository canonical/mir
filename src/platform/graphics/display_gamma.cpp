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

#include "mir/graphics/display_gamma.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;

mg::DisplayGamma::DisplayGamma(std::vector<uint16_t> const& red,
                               std::vector<uint16_t> const& green,
                               std::vector<uint16_t> const& blue) :
    red_(red),
    green_(green),
    blue_(blue)
{
    throw_if_lut_size_mismatch();
}

mg::DisplayGamma::DisplayGamma(std::vector<uint16_t>&& red,
                               std::vector<uint16_t>&& green,
                               std::vector<uint16_t>&& blue) :
    red_(std::move(red)),
    green_(std::move(green)),
    blue_(std::move(blue))
{
    throw_if_lut_size_mismatch();
}

void mg::DisplayGamma::throw_if_lut_size_mismatch() const
{
    if (red_.size() != green_.size() ||
        green_.size() != blue_.size())
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Different gamma LUT sizes"));
    }
}

uint16_t const* mg::DisplayGamma::red() const
{
    return red_.data();
}

uint16_t const* mg::DisplayGamma::green() const
{
    return green_.data();
}

uint16_t const* mg::DisplayGamma::blue() const
{
    return blue_.data();
}

uint32_t mg::DisplayGamma::size() const
{
    return red_.size();
}
