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
#include "mir_toolkit/events/input/input_event.h"
#include "src/platforms/mesa/server/x11/input/dispatchable.h"
#include "mir/test/doubles/mock_input_sink.h"
#include "mir/test/doubles/mock_x11.h"
#include "src/server/input/default_event_builder.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtd = mir::test::doubles;

using namespace ::testing;

namespace
{

struct X11DispatchableTest : ::testing::Test
{
    NiceMock<mtd::MockInputSink> mock_input_sink;
    NiceMock<mtd::MockX11> mock_x11;
    std::unique_ptr<mir::input::DefaultEventBuilder> builder =
        std::make_unique<mir::input::DefaultEventBuilder>(0);

    mir::input::X::XDispatchable x11_dispatchable{
        std::shared_ptr<::Display>(
            XOpenDisplay(nullptr),
            [](::Display* display) { XCloseDisplay(display); }), 0};
};

}

TEST_F(X11DispatchableTest, dispatches_input_events_to_sink)
{
    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_x11.fake_x11.keypress_event_return),
                       Return(1)));

    EXPECT_CALL(mock_input_sink, handle_input(_))
        .Times(Exactly(1));

    x11_dispatchable.set_input_sink(&mock_input_sink, builder.get());
    x11_dispatchable.dispatch(mir::dispatch::FdEvent::readable);
}

TEST_F(X11DispatchableTest, grabs_keyboard)
{
    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_x11.fake_x11.focus_in_event_return),
                       Return(1)));

    EXPECT_CALL(mock_x11, XGrabKeyboard(_,_,_,_,_,_))
        .Times(Exactly(1));
    EXPECT_CALL(mock_input_sink, handle_input(_))
        .Times(Exactly(0));

    x11_dispatchable.set_input_sink(&mock_input_sink, builder.get());
    x11_dispatchable.dispatch(mir::dispatch::FdEvent::readable);
}

TEST_F(X11DispatchableTest, ungrabs_keyboard)
{
    ON_CALL(mock_x11, XNextEvent(_,_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_x11.fake_x11.focus_out_event_return),
                       Return(1)));

    EXPECT_CALL(mock_x11, XUngrabKeyboard(_,_))
        .Times(Exactly(1));
    EXPECT_CALL(mock_input_sink, handle_input(_))
        .Times(Exactly(0));

    x11_dispatchable.set_input_sink(&mock_input_sink, builder.get());
    x11_dispatchable.dispatch(mir::dispatch::FdEvent::readable);
}
