/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_VIEWABLE_AREA_H_
#define MIR_TEST_DOUBLES_MOCK_VIEWABLE_AREA_H_

#include "mir/graphics/viewable_area.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockViewableArea : public graphics::ViewableArea
{
public:
    MOCK_CONST_METHOD0(view_area, geometry::Rectangle ());
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_MOCK_VIEWABLE_AREA_H_ */
