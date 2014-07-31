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

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace geom = mir::geometry;

namespace
{

struct NoOutputsServerConfig : mtf::StubbedServerConfiguration
{
    NoOutputsServerConfig()
        : mtf::StubbedServerConfiguration(std::vector<geom::Rectangle>{})
    {
    }
};

using ServerWithoutActiveOutputs = mtf::BasicClientServerFixture<NoOutputsServerConfig>;

}

TEST_F(ServerWithoutActiveOutputs, creates_valid_client_surface)
{
    using namespace testing;

    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    auto const surface = mir_connection_create_surface_sync(connection, &request_params);

    EXPECT_THAT(mir_surface_is_valid(surface), Eq(mir_true))
        << mir_surface_get_error_message(surface);

    mir_surface_release_sync(surface);
}
