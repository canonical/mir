/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_SURFACE_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_SURFACE_H_

#include <mir/input/surface.h>
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
    MOCK_METHOD(std::string, name, (), (const override));
    MOCK_METHOD(geometry::Rectangle, input_bounds, (), (const override));
    MOCK_METHOD(bool, input_area_contains, (geometry::Point const&), (const override));
    MOCK_METHOD(std::shared_ptr<graphics::CursorImage>, cursor_image, (), (const override));
    MOCK_METHOD(input::InputReceptionMode, reception_mode, (), (const override));
    MOCK_METHOD(void, consume, (std::shared_ptr<MirEvent const> const&), (override));
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_INPUT_SURFACE_H_ */
