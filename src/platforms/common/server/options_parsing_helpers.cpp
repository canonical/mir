/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "options_parsing_helpers.h"
#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace mgc = mir::graphics::common;
namespace geom = mir::geometry;

namespace
{
auto parse_scale(std::string const& str) -> float
{
    try
    {
        size_t num_end = 0;
        float const value = std::stof(str, &num_end);
        if (num_end != str.size())
            BOOST_THROW_EXCEPTION(std::runtime_error("Scale \"" + str + "\" is not a valid float"));
        if (value <= 0.000001)
            BOOST_THROW_EXCEPTION(std::runtime_error("Scale must be greater than zero"));
        return value;
    }
    catch (std::invalid_argument const &)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Scale \"" + str + "\" is not a valid float"));
    }
    catch (std::out_of_range const &)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Scale \"" + str + "\" is out of range"));
    }
}

auto parse_size_dimension(std::string const& str) -> int
{
    try
    {
        size_t num_end = 0;
        int const value = std::stoi(str, &num_end);
        if (num_end != str.size())
            BOOST_THROW_EXCEPTION(std::runtime_error("Output dimension \"" + str + "\" is not a valid number"));
        if (value <= 0)
            BOOST_THROW_EXCEPTION(std::runtime_error("Output dimensions must be greater than zero"));
        return value;
    }
    catch (std::invalid_argument const &)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Output dimension \"" + str + "\" is not a valid number"));
    }
    catch (std::out_of_range const &)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Output dimension \"" + str + "\" is out of range"));
    }
}
}


auto mgc::parse_size(std::string const& str) -> geom::Size
{
    auto const x = str.find('x'); // "x" between width and height
    if (x == std::string::npos || x <= 0 || x >= str.size() - 1)
        BOOST_THROW_EXCEPTION(std::runtime_error("Output size \"" + str + "\" does not have two dimensions"));
    return geom::Size{
        parse_size_dimension(str.substr(0, x)),
        parse_size_dimension(str.substr(x + 1))};
}

auto mgc::parse_size_with_scale(std::string const& str) -> std::tuple<mir::geometry::Size, float>
{
    auto const x = str.find('x'); // "x" between width and height
    if (x == std::string::npos || x <= 0 || x >= str.size() - 1)
        BOOST_THROW_EXCEPTION(std::runtime_error("Output size \"" + str + "\" does not have two dimensions"));
    auto const scale_start = str.find('^'); // start of output scale
    float scale = 1.0f;
    if (scale_start != std::string::npos)
    {
        if (scale_start >= str.size() - 1)
            BOOST_THROW_EXCEPTION(std::runtime_error("In \"" + str + "\", '^' is not followed by a scale"));
        scale = parse_scale(str.substr(scale_start + 1, std::string::npos));
    }
    return {geom::Size{
            parse_size_dimension(str.substr(0, x)),
            parse_size_dimension(str.substr(x + 1, scale_start - x - 1))},
        scale};
}
