/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/frontend/client_buffer_tracker.h"
#include "mir/compositor/buffer_id.h"

#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mc = mir::compositor;

TEST(ClientBufferTracker, just_added_buffer_is_known_by_client)
{
    using namespace testing;

    mf::ClientBufferTracker tracker;
    mc::BufferID id{5};

    tracker.add(id);
    EXPECT_TRUE(tracker.client_has(id));
}

TEST(ClientBufferTracker, unadded_buffer_is_unknown_by_client)
{
    using namespace testing;

    mf::ClientBufferTracker tracker;

    tracker.add(mc::BufferID{5});
    EXPECT_FALSE(tracker.client_has(mc::BufferID{6}));
}
