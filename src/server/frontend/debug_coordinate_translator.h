/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_FRONTEND_DEBUG_COORDINATE_TRANSLATOR_H_
#define MIR_FRONTEND_DEBUG_COORDINATE_TRANSLATOR_H_

#include "coordinate_translator.h"

namespace mir
{
namespace frontend
{
class DebugCoordinateTranslator : public CoordinateTranslator
{
public:
    geometry::Point surface_to_screen(std::shared_ptr<Surface> surface, uint32_t x, uint32_t y);
};

}
}

#endif // MIR_FRONTEND_DEBUG_COORDINATE_TRANSLATOR_H_
