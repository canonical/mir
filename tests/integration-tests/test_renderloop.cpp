/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/display.h"
#include "mir/graphics/renderer.h"
#include "mir/display_server.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_doubles/mock_display.h"
#include "mir_test_doubles/mock_display_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{

ACTION_P(InvokeArgWithParam, param)
{
    arg0(param);
}

}

TEST_F(BespokeDisplayServerTestFixture, notify_sync_and_see_paint)
{
    struct Server : TestingServerConfiguration
    {
        void exec(mir::DisplayServer* display_server)
        {
            using namespace testing;

            mtd::MockDisplay display;
            NiceMock<mtd::MockDisplayBuffer> display_buffer;

            ON_CALL(display_buffer, view_area())
                .WillByDefault(Return(geom::Rectangle()));

            EXPECT_CALL(display, for_each_display_buffer(_))
                .WillOnce(InvokeArgWithParam(ByRef(display_buffer)));

            EXPECT_CALL(display_buffer, post_update())
                .WillOnce(Return(true));

            display_server->render(&display);
        }
    } server_processing;

    launch_server_process(server_processing);
}
