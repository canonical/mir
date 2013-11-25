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

#include "mir/frontend/session_mediator_report.h"
#include "src/server/frontend/session_mediator.h"
#include "src/server/frontend/resource_cache.h"
#include "src/server/scene/application_session.h"
#include "mir/frontend/shell.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"

#include "mir_test_doubles/null_display_changer.h"
#include "mir_test_doubles/mock_session.h"
#include "mir_test_doubles/stub_shell.h"
#include "mir_test_doubles/null_platform.h"
#include "mir_test_doubles/null_event_sink.h"
#include "mir_test_doubles/stub_buffer_allocator.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;
namespace msh = mir::shell;
namespace mtd = mir::test::doubles;

namespace
{

struct SessionMediatorAndroidTest : public ::testing::Test
{
    SessionMediatorAndroidTest()
        : shell{std::make_shared<mtd::StubShell>()},
          graphics_platform{std::make_shared<mtd::NullPlatform>()},
          display_changer{std::make_shared<mtd::NullDisplayChanger>()},
          surface_pixel_formats{geom::PixelFormat::argb_8888, geom::PixelFormat::xrgb_8888},
          report{std::make_shared<mf::NullSessionMediatorReport>()},
          resource_cache{std::make_shared<mf::ResourceCache>()},
          mediator{shell, graphics_platform, display_changer,
                   surface_pixel_formats, report,
                   std::make_shared<mtd::NullEventSink>(),
                   resource_cache},
          null_callback{google::protobuf::NewPermanentCallback(google::protobuf::DoNothing)}
    {
    }

    std::shared_ptr<mtd::StubShell> const shell;
    std::shared_ptr<mtd::NullPlatform> const graphics_platform;
    std::shared_ptr<mf::DisplayChanger> const display_changer;
    std::vector<geom::PixelFormat> const surface_pixel_formats;
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
