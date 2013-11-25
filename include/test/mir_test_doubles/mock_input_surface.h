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
    MockInputSurface()
    {
        using namespace testing;
        ON_CALL(*this, position())
            .WillByDefault(
                Return(geometry::Point{}));
        ON_CALL(*this, size())
            .WillByDefault(
                Return(geometry::Size{}));
        static std::string n;
        ON_CALL(*this, name())
            .WillByDefault(testing::ReturnRef(n));
    }
    ~MockInputSurface() noexcept {}
    MOCK_CONST_METHOD0(position, geometry::Point());
    MOCK_CONST_METHOD0(size, geometry::Size());
    MOCK_CONST_METHOD0(name, std::string const&());
    MOCK_CONST_METHOD1(contains, bool(geometry::Point const&));
};

typedef ::testing::NiceMock<MockInputSurface> StubInputSurface;
}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_INPUT_SURFACE_H_ */
