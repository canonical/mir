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

#ifndef MIR_TEST_DOUBLES_MOCK_SURFACE_INFO_H_
#define MIR_TEST_DOUBLES_MOCK_SURFACE_INFO_H_

#include "mir/surfaces/surface_info.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockSurfaceInfo : public surfaces::SurfaceInfo 
{
public:
    MockSurfaceInfo()
    {
        ON_CALL(*this, top_left())
            .WillByDefault(testing::Return(geometry::Point{geometry::X{}, geometry::Y{}}));
        ON_CALL(*this, size())
            .WillByDefault(testing::Return(geometry::Size{geometry::Width{}, geometry::Height{}}));
        static std::string n;
        ON_CALL(*this, name())
            .WillByDefault(testing::ReturnRef(n));
    }
    ~MockSurfaceInfo() noexcept {}
    MOCK_CONST_METHOD0(top_left, geometry::Point());
    MOCK_METHOD1(set_top_left, void(geometry::Point));
    MOCK_CONST_METHOD0(size, geometry::Size());
    MOCK_CONST_METHOD0(name, std::string const&());
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_SURFACE_INFO_H_ */
