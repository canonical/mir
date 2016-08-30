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

mg::DisplayGamma::DisplayGamma(uint16_t const* red,
                               uint16_t const* green,
                               uint16_t const* blue,
                               uint32_t size) :
    red_  (std::vector<uint16_t>(red,   red   + size)),
    green_(std::vector<uint16_t>(green, green + size)),
    blue_ (std::vector<uint16_t>(blue,  blue  + size))
{
}

mg::DisplayGamma::DisplayGamma(std::string const& bytes_red,
                               std::string const& bytes_green,
                               std::string const& bytes_blue) :
    red_  (bytes_red.size() / 2),
    green_(bytes_red.size() / 2),
    blue_ (bytes_red.size() / 2)
{
    if (bytes_red.size()   != bytes_green.size() ||
        bytes_green.size() != bytes_blue.size())
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Mismatch gamma LUT size"));
    }

    std::copy(std::begin(bytes_red),   std::end(bytes_red),   reinterpret_cast<char*>(red_.data()));
    std::copy(std::begin(bytes_green), std::end(bytes_green), reinterpret_cast<char*>(green_.data()));
    std::copy(std::begin(bytes_blue),  std::end(bytes_blue),  reinterpret_cast<char*>(blue_.data()));
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
