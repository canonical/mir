/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "mir/events/event_private.h"
#include "mir_toolkit/event.h"
#include "src/platforms/mesa/server/x11/input/dispatchable.h"
#include "mir/test/doubles/mock_input_sink.h"
#include "mir/test/doubles/mock_x11.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mix = mi::X;
namespace mtd = mir::test::doubles;

using namespace ::testing;

namespace
{

struct X11DispatchableTest : ::testing::Test
{
	X11DispatchableTest()
	{
	}

    mix::XDispatchable x11_dispatchable{0};
    NiceMock<mtd::MockInputSink> mock_input_sink;
    NiceMock<mtd::MockX11> mock_x11;
};

}

TEST_F(X11DispatchableTest, dispatches_input_events_to_sink)
{
    ON_CALL(mock_x11, XNextEvent(_,_))
    .WillByDefault(DoAll(SetArgPointee<1>(mock_x11.fake_x11.event_return),
                   Return(1)));

    EXPECT_CALL(mock_input_sink, handle_input(_));

    x11_dispatchable.set_input_sink(&mock_input_sink);
    x11_dispatchable.dispatch(0);
}
