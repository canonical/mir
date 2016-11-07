/*
 * Copyright © 2015 Canonical Ltd.
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

#include "../buffer_stream_arrangement.h"
#include "mir_toolkit/mir_presentation_chain.h"
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace geom = mir::geometry;
using namespace std::literals::chrono_literals;
using namespace testing;

typedef mt::BufferStreamArrangementBase BufferStreamArrangementStaging;
TEST_F(BufferStreamArrangementStaging, can_set_stream_logical_and_physical_size)
{
    geom::Size logical_size { 233, 111 };
    geom::Size physical_size { 133, 222 };
    mt::Stream stream(connection, physical_size, { {0, 0}, logical_size } );

    auto change_spec = mir_connection_create_spec_for_changes(connection);
    mir_surface_spec_add_buffer_stream(change_spec, 0, 0, logical_size.width.as_int(), logical_size.height.as_int(), stream.handle());
    mir_surface_apply_spec(surface, change_spec);
    mir_surface_spec_release(change_spec);

    std::vector<mt::RelativeRectangle> positions { { {0,0}, logical_size, physical_size } };
    EXPECT_TRUE(ordering->wait_for_positions_within(positions, 5s))
         << "timed out waiting to see the compositor post the streams in the right arrangement";
}

TEST_F(BufferStreamArrangementStaging, can_setting_stream_physical_size_doesnt_affect_logical)
{
    geom::Size logical_size { 233, 111 };
    geom::Size original_physical_size { 133, 222 };
    geom::Size changed_physical_size { 133, 222 };
    mt::Stream stream(connection, original_physical_size, { {0, 0}, logical_size } );

    auto change_spec = mir_connection_create_spec_for_changes(connection);
    mir_surface_spec_add_buffer_stream(change_spec, 0, 0, logical_size.width.as_int(), logical_size.height.as_int(), stream.handle());
    mir_surface_apply_spec(surface, change_spec);
    mir_surface_spec_release(change_spec);

    stream.set_size(changed_physical_size);
    //submits the original_buffer_size buffer, getting changed_physical_size buffer as current
    stream.swap_buffers();
    //submits the changed_buffer_size buffer so server can see
    stream.swap_buffers();

    std::vector<mt::RelativeRectangle> positions { { {0,0}, logical_size, changed_physical_size } };
    EXPECT_TRUE(ordering->wait_for_positions_within(positions, 5s))
         << "timed out waiting to see the compositor post the streams in the right arrangement";
}

TEST_F(BufferStreamArrangementStaging, stream_size_reflects_current_buffer_physical_size)
{
    geom::Size logical_size { 233, 111 };
    geom::Size original_physical_size { 133, 222 };
    geom::Size changed_physical_size { 133, 222 };
    mt::Stream stream(connection, original_physical_size, { {0, 0}, logical_size } );

    auto change_spec = mir_connection_create_spec_for_changes(connection);
    mir_surface_spec_add_buffer_stream(change_spec, 0, 0, logical_size.width.as_int(), logical_size.height.as_int(), stream.handle());
    mir_surface_apply_spec(surface, change_spec);
    mir_surface_spec_release(change_spec);

    EXPECT_THAT(stream.physical_size(), Eq(original_physical_size));
    streams.back()->set_size(changed_physical_size);
    EXPECT_THAT(stream.physical_size(), Eq(changed_physical_size));
    streams.back()->swap_buffers();
    EXPECT_THAT(stream.physical_size(), Eq(changed_physical_size));
}
