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
#include "mir/frontend/application_mediator_report.h"
#include "mir/frontend/application_mediator.h"
#include "mir/frontend/resource_cache.h"
#include "mir/shell/application_session.h"
#include "mir/shell/session_store.h"
#include "mir/shell/surface_factory.h"
#include "mir/graphics/display.h"
#include "mir/graphics/drm_authenticator.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include "mir_test_doubles/null_display.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;
namespace msh = mir::shell;
namespace mtd = mir::test::doubles;

namespace
{

/*
 * TODO: Fix design so that it's possible to unit-test ApplicationMediator
 * without having to create doubles for classes so deep in its dependency
 * hierarchy.
 *
 * In particular, it would be nice if msh::ApplicationSession was stubable/mockable.
 */

class StubSurfaceFactory : public msh::SurfaceFactory
{
 public:
    std::shared_ptr<msh::Surface> create_surface(const msh::SurfaceCreationParameters& /*params*/)
    {
        return std::shared_ptr<msh::Surface>();
    }
};

class StubSessionStore : public msh::SessionStore
{
public:
    StubSessionStore()
        : factory{std::make_shared<StubSurfaceFactory>()}
    {
    }

    std::shared_ptr<msh::Session> open_session(std::string const& /*name*/)
    {
        return std::make_shared<msh::ApplicationSession>(factory, "stub");
    }

    void close_session(std::shared_ptr<msh::Session> const& /*session*/) {}

    void shutdown() {}
    void tag_session_with_lightdm_id(std::shared_ptr<msh::Session> const&, int) {}
    void focus_session_with_lightdm_id(int) {}

    std::shared_ptr<msh::SurfaceFactory> factory;
};

class StubGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
public:
    std::shared_ptr<mc::Buffer> alloc_buffer(mc::BufferProperties const&)
    {
        return std::shared_ptr<mc::Buffer>();
    }

    virtual std::vector<geom::PixelFormat> supported_pixel_formats()
    {
        return std::vector<geom::PixelFormat>();
    }
};

class MockAuthenticatingPlatform : public mg::Platform, public mg::DRMAuthenticator
{
 public:
    std::shared_ptr<mc::GraphicBufferAllocator> create_buffer_allocator(
            const std::shared_ptr<mg::BufferInitializer>& /*buffer_initializer*/)
    {
        return std::shared_ptr<StubGraphicBufferAllocator>();
    }

    std::shared_ptr<mg::Display> create_display()
    {
        return std::make_shared<mtd::NullDisplay>();
    }

    std::shared_ptr<mg::PlatformIPCPackage> get_ipc_package()
    {
        return std::make_shared<mg::PlatformIPCPackage>();
    }

    MOCK_METHOD1(drm_auth_magic, void(drm_magic_t));
};

}

struct ApplicationMediatorGBMTest : public ::testing::Test
{
    ApplicationMediatorGBMTest()
        : session_store{std::make_shared<StubSessionStore>()},
          mock_platform{std::make_shared<MockAuthenticatingPlatform>()},
          graphics_display{std::make_shared<mtd::NullDisplay>()},
          buffer_allocator{std::make_shared<StubGraphicBufferAllocator>()},
          report{std::make_shared<mf::NullApplicationMediatorReport>()},
          resource_cache{std::make_shared<mf::ResourceCache>()},
          mediator{session_store, mock_platform, graphics_display,
                   buffer_allocator, report, resource_cache},
          null_callback{google::protobuf::NewPermanentCallback(google::protobuf::DoNothing)}
    {
    }

    std::shared_ptr<msh::SessionStore> const session_store;
    std::shared_ptr<MockAuthenticatingPlatform> const mock_platform;
    std::shared_ptr<mg::Display> const graphics_display;
    std::shared_ptr<mc::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<mf::ApplicationMediatorReport> const report;
    std::shared_ptr<mf::ResourceCache> const resource_cache;
    mf::ApplicationMediator mediator;

    std::unique_ptr<google::protobuf::Closure> null_callback;
};

TEST_F(ApplicationMediatorGBMTest, drm_auth_magic_uses_drm_authenticator)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    drm_magic_t const drm_magic{0x10111213};
    int const no_error{0};

    EXPECT_CALL(*mock_platform, drm_auth_magic(drm_magic))
        .Times(1);

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    mp::DRMMagic magic;
    mp::DRMAuthMagicStatus status;
    magic.set_magic(drm_magic);

    mediator.drm_auth_magic(nullptr, &magic, &status, null_callback.get());

    EXPECT_EQ(no_error, status.status_code());
}

TEST_F(ApplicationMediatorGBMTest, drm_auth_magic_sets_status_code_on_error)
{
    using namespace testing;

    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    drm_magic_t const drm_magic{0x10111213};
    int const error_number{667};

    EXPECT_CALL(*mock_platform, drm_auth_magic(drm_magic))
        .WillOnce(Throw(::boost::enable_error_info(std::exception())
            << boost::errinfo_errno(error_number)));

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    mp::DRMMagic magic;
    mp::DRMAuthMagicStatus status;
    magic.set_magic(drm_magic);

    mediator.drm_auth_magic(nullptr, &magic, &status, null_callback.get());

    EXPECT_EQ(error_number, status.status_code());
}
