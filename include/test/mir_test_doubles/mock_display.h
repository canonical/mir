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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_H_

#include "mir/graphics/display.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockDisplay : public graphics::Display
{
public:
    MOCK_CONST_METHOD0(view_area, geometry::Rectangle ());
    MOCK_METHOD1(for_each_display_buffer, void (std::function<void(graphics::DisplayBuffer&)> const&));
    MOCK_METHOD0(configuration, std::shared_ptr<graphics::DisplayConfiguration>());
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_H_ */
