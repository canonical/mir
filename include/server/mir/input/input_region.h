/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_INPUT_REGION_H_
#define MIR_INPUT_INPUT_REGION_H_

#include <stdint.h>

namespace mir
{
namespace input
{

class InputRegion
{
public:
    virtual ~InputRegion() = default;

    virtual bool contains_point(uint32_t x, uint32_t y) const = 0;

protected:
    InputRegion() = default;
    InputRegion(InputRegion const&) = delete;
    InputRegion& operator=(InputRegion const&) = delete;
};

}
}

#endif // MIR_INPUT_INPUT_REGION
