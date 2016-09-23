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

#ifndef MIR_GRAPHICS_GAMMA_CURVES_H_
#define MIR_GRAPHICS_GAMMA_CURVES_H_

#include <cstdint>
#include <vector>

namespace mir
{
namespace graphics
{

class GammaCurves
{
public:
    GammaCurves() = default;

    GammaCurves(std::vector<uint16_t> const& red,
                std::vector<uint16_t> const& green,
                std::vector<uint16_t> const& blue);

    std::vector<uint16_t> red;
    std::vector<uint16_t> green;
    std::vector<uint16_t> blue;
};

}
}

#endif
