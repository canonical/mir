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

namespace
{

class StubGraphicBufferAllocator : public mtd::StubBufferAllocator
{
public:
    std::vector<MirPixelFormat> supported_pixel_formats() override
    {
        return pixel_formats;
    }

    static std::vector<MirPixelFormat> const pixel_formats;
};

std::vector<MirPixelFormat> const StubGraphicBufferAllocator::pixel_formats{
    mir_pixel_format_argb_8888,
    mir_pixel_format_xbgr_8888,
    mir_pixel_format_bgr_888
};

class StubPlatform : public mtd::NullPlatform
{
public:
    std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator() override
    {
        return std::make_shared<StubGraphicBufferAllocator>();
    }
};

struct ServerConfig : mtf::TestingServerConfiguration
{
    std::shared_ptr<mg::Platform> the_graphics_platform() override
    {
        if (!platform)
            platform = std::make_shared<StubPlatform>();

        return platform;
    }

    std::shared_ptr<mg::Platform> platform;
};

using AvailableSurfaceFormats = mtf::BasicClientServerFixture<ServerConfig>;

}

TEST_F(AvailableSurfaceFormats, reach_clients)
{
    using namespace testing;

    std::vector<MirPixelFormat> formats(4);
    unsigned int returned_format_size = 0;

    mir_connection_get_available_surface_formats(
        connection, formats.data(), formats.size(), &returned_format_size);

    formats.resize(returned_format_size);

    EXPECT_THAT(formats, ContainerEq(StubGraphicBufferAllocator::pixel_formats));
}
