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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_SURFACE_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_SURFACE_H_

#include "mir/input/surface.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockInputSurface : public input::Surface
{
public:
    ~MockInputSurface() noexcept {}
    MOCK_CONST_METHOD0(name, std::string());
    MOCK_CONST_METHOD0(input_bounds, geometry::Rectangle());
    MOCK_CONST_METHOD1(input_area_contains, bool(geometry::Point const&));
    MOCK_CONST_METHOD0(input_channel, std::shared_ptr<input::InputChannel>());
    MOCK_CONST_METHOD0(cursor_image, std::shared_ptr<graphics::CursorImage>());
    MOCK_CONST_METHOD0(reception_mode, input::InputReceptionMode());
    MOCK_METHOD1(consume, void(MirEvent const&));
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_INPUT_SURFACE_H_ */
