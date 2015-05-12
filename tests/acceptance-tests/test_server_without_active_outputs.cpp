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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_framework/any_surface.h"

#include "mir/geometry/rectangle.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace geom = mir::geometry;

namespace
{
struct ServerWithoutActiveOutputs : mtf::ConnectedClientHeadlessServer
{
    void SetUp() override
    {
        initial_display_layout(std::vector<geom::Rectangle>{});
        mtf::ConnectedClientHeadlessServer::SetUp();
    }
};
}

TEST_F(ServerWithoutActiveOutputs, creates_valid_client_surface)
{
    using namespace testing;

    auto const surface = mtf::make_any_surface(connection);

    EXPECT_THAT(mir_surface_is_valid(surface), Eq(true))
        << mir_surface_get_error_message(surface);

    mir_surface_release_sync(surface);
}
