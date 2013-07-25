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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/frontend/session_mediator_report.h"
#include "mir/frontend/session_mediator.h"
#include "mir/frontend/resource_cache.h"
#include "mir/shell/application_session.h"
#include "mir/frontend/shell.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"

#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/mock_session.h"
#include "mir_test_doubles/stub_shell.h"
#include "mir_test_doubles/null_platform.h"
#include "mir_test_doubles/null_event_sink.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;
namespace msh = mir::shell;
namespace mtd = mir::test::doubles;

namespace
{

class StubGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
public:
    std::shared_ptr<mg::Buffer> alloc_buffer(mc::BufferProperties const&)
    {
        return std::shared_ptr<mg::Buffer>();
    }

    virtual std::vector<geom::PixelFormat> supported_pixel_formats()
    {
        return std::vector<geom::PixelFormat>();
    }
};

struct SessionMediatorAndroidTest : public ::testing::Test
{
    SessionMediatorAndroidTest()
        : shell{std::make_shared<mtd::StubShell>()},
          graphics_platform{std::make_shared<mtd::NullPlatform>()},
          graphics_display{std::make_shared<mtd::NullDisplay>()},
          buffer_allocator{std::make_shared<StubGraphicBufferAllocator>()},
          report{std::make_shared<mf::NullSessionMediatorReport>()},
          resource_cache{std::make_shared<mf::ResourceCache>()},
          mediator{shell, graphics_platform, graphics_display,
                   buffer_allocator, report,
                   std::make_shared<mtd::NullEventSink>(),
                   resource_cache},
          null_callback{google::protobuf::NewPermanentCallback(google::protobuf::DoNothing)}
    {
    }

    std::shared_ptr<mtd::StubShell> const shell;
    std::shared_ptr<mtd::NullPlatform> const graphics_platform;
    std::shared_ptr<mg::Display> const graphics_display;
    std::shared_ptr<mc::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<mf::SessionMediatorReport> const report;
    std::shared_ptr<mf::ResourceCache> const resource_cache;
    mf::SessionMediator mediator;

    std::unique_ptr<google::protobuf::Closure> null_callback;
};

}

TEST_F(SessionMediatorAndroidTest, drm_auth_magic_throws)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    mp::DRMMagic magic;
    magic.set_magic(0x10111213);

    EXPECT_THROW({
        mediator.drm_auth_magic(nullptr, &magic, nullptr, null_callback.get());
    }, std::logic_error);
}
