/*
 * Copyright © 2012-2015 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/test/doubles/null_platform.h"
#include "mir/test/doubles/stub_buffer_allocator.h"

#include "mir_test_framework/basic_client_server_fixture.h"
#include "mir_test_framework/testing_server_configuration.h"

#include "mir_toolkit/mir_client_library.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;

using AvailableSurfaceFormats = mtf::BasicClientServerFixture<mtf::TestingServerConfiguration>;

TEST_F(AvailableSurfaceFormats, reach_clients)
{
    using namespace testing;

    std::vector<MirPixelFormat> formats(4);
    unsigned int returned_format_size = 0;

    mir_connection_get_available_surface_formats(
        connection, formats.data(), formats.size(), &returned_format_size);

    formats.resize(returned_format_size);

    EXPECT_THAT(
        formats,
        ContainerEq(server_config().the_buffer_allocator()->supported_pixel_formats()));
}
