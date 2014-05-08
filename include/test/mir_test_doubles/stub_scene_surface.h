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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_SCENE_SURFACE_H_
#define MIR_TEST_DOUBLES_STUB_SCENE_SURFACE_H_

#include "mir_test_doubles/mock_scene_surface.h"
#include "mir_test_doubles/stub_input_channel.h"
#include "mir_test/fake_shared.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class StubSceneSurface :
    public ::testing::NiceMock<MockSceneSurface>
{
public:
    StubInputChannel channel;
    int fd;
    StubSceneSurface(int fd)
        : channel(fd), fd(fd)
    {
        using namespace ::testing;
        ON_CALL(*this, input_channel()).WillByDefault(Return(mir::test::fake_shared(channel)));
        ON_CALL(*this, reception_mode()).WillByDefault(Return(mir::input::InputReceptionMode::normal));
        ON_CALL(*this, name()).WillByDefault(Return("stub_surface"));
        ON_CALL(*this, size()).WillByDefault(Return(mir::geometry::Size{60,90}));
        ON_CALL(*this, top_left()).WillByDefault(Return(mir::geometry::Point{0,0}));

    }
};

}
}
}

#endif


