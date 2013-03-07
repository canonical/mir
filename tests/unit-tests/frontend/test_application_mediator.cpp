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

#include "mir/surfaces/buffer_bundle.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/frontend/application_mediator_report.h"
#include "mir/frontend/application_mediator.h"
#include "mir/frontend/resource_cache.h"
#include "mir/shell/application_session.h"
#include "mir/shell/session_store.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/surfaces/surface.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/mock_session.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test/fake_shared.h"

#include "src/surfaces/proxy_surface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{

class DestructionRecordingSession : public testing::NiceMock<mtd::MockSession>
{
public:
    DestructionRecordingSession()
    {
        using namespace ::testing;
        destroyed = false;
        
        ON_CALL(*this, create_surface(_)).WillByDefault(Return(test_surface_id));
        ON_CALL(*this, get_surface(_)).WillByDefault(Return(mt::fake_shared(mock_surface)));
        
        ON_CALL(mock_surface, size()).WillByDefault(Return(geom::Size()));
        ON_CALL(mock_surface, pixel_format()).WillByDefault(Return(geom::PixelFormat()));
        ON_CALL(mock_surface, client_buffer()).WillByDefault(Return(mt::fake_shared(stub_buffer)));
    }

    ~DestructionRecordingSession() { destroyed = true; }

    static bool destroyed;
    static msh::SurfaceId test_surface_id;
    testing::NiceMock<mtd::MockSurface> mock_surface;
    mtd::StubBuffer stub_buffer;
};

msh::SurfaceId DestructionRecordingSession::test_surface_id{29};

bool DestructionRecordingSession::destroyed{true};

class StubSessionStore : public msh::SessionStore
{
public:
    StubSessionStore()
    {
    }

    std::shared_ptr<msh::Session> open_session(std::string const& /*name*/)
    {
        return std::make_shared<DestructionRecordingSession>();
    }

    void close_session(std::shared_ptr<msh::Session> const& /*session*/) {}

    void shutdown() {}
    void tag_session_with_lightdm_id(std::shared_ptr<msh::Session> const&, int) {}
    void focus_session_with_lightdm_id(int) {}
};

class MockGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
public:
    MockGraphicBufferAllocator()
    {
        ON_CALL(*this, supported_pixel_formats())
            .WillByDefault(testing::Return(std::vector<geom::PixelFormat>()));
    }

    std::shared_ptr<mc::Buffer> alloc_buffer(mc::BufferProperties const&)
    {
        return std::shared_ptr<mc::Buffer>();
    }

    MOCK_METHOD0(supported_pixel_formats, std::vector<geom::PixelFormat>());
};

class StubPlatform : public mg::Platform
{
 public:
    std::shared_ptr<mc::GraphicBufferAllocator> create_buffer_allocator(
            const std::shared_ptr<mg::BufferInitializer>& /*buffer_initializer*/)
    {
        return std::shared_ptr<mc::GraphicBufferAllocator>();
    }

    std::shared_ptr<mg::Display> create_display()
    {
        return std::make_shared<mtd::NullDisplay>();
    }

    std::shared_ptr<mg::PlatformIPCPackage> get_ipc_package()
    {
        return std::make_shared<mg::PlatformIPCPackage>();
    }
};

}

struct ApplicationMediatorTest : public ::testing::Test
{
    ApplicationMediatorTest()
        : session_store{std::make_shared<StubSessionStore>()},
          graphics_platform{std::make_shared<StubPlatform>()},
          graphics_display{std::make_shared<mtd::NullDisplay>()},
          buffer_allocator{std::make_shared<testing::NiceMock<MockGraphicBufferAllocator>>()},
          report{std::make_shared<mf::NullApplicationMediatorReport>()},
          resource_cache{std::make_shared<mf::ResourceCache>()},
          mediator{session_store, graphics_platform, graphics_display,
                   buffer_allocator, report, resource_cache},
          null_callback{google::protobuf::NewPermanentCallback(google::protobuf::DoNothing)}
    {
    }

    std::shared_ptr<msh::SessionStore> const session_store;
    std::shared_ptr<mg::Platform> const graphics_platform;
    std::shared_ptr<mg::Display> const graphics_display;
    std::shared_ptr<testing::NiceMock<MockGraphicBufferAllocator>> const buffer_allocator;
    std::shared_ptr<mf::ApplicationMediatorReport> const report;
    std::shared_ptr<mf::ResourceCache> const resource_cache;
    mf::ApplicationMediator mediator;

    std::unique_ptr<google::protobuf::Closure> null_callback;
};


TEST_F(ApplicationMediatorTest, disconnect_releases_session)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    EXPECT_FALSE(DestructionRecordingSession::destroyed);

    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());

    EXPECT_TRUE(DestructionRecordingSession::destroyed);
}

TEST_F(ApplicationMediatorTest, calling_methods_before_connect_throws)
{
    EXPECT_THROW({
        mp::SurfaceParameters request;
        mp::Surface response;

        mediator.create_surface(nullptr, &request, &response, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mp::SurfaceId request;
        mp::Buffer response;

        mediator.next_buffer(nullptr, &request, &response, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mp::SurfaceId request;

        mediator.release_surface(nullptr, &request, nullptr, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mp::DRMMagic request;
        mp::DRMAuthMagicStatus response;

        mediator.drm_auth_magic(nullptr, &request, &response, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
    }, std::logic_error);
}

TEST_F(ApplicationMediatorTest, calling_methods_after_connect_works)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    {
        mp::SurfaceParameters request;
        mp::Surface response;

        mediator.create_surface(nullptr, &request, &response, null_callback.get());
    }
    {
        mp::SurfaceId request;
        mp::Buffer response;

        mediator.next_buffer(nullptr, &request, &response, null_callback.get());
    }
    {
        mp::SurfaceId request;

        mediator.release_surface(nullptr, &request, nullptr, null_callback.get());
    }

    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
}

TEST_F(ApplicationMediatorTest, calling_methods_after_disconnect_throws)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());

    EXPECT_THROW({
        mp::SurfaceParameters surface_parameters;
        mp::Surface surface;

        mediator.create_surface(nullptr, &surface_parameters, &surface, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mp::SurfaceId request;
        mp::Buffer response;

        mediator.next_buffer(nullptr, &request, &response, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mp::SurfaceId request;

        mediator.release_surface(nullptr, &request, nullptr, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mp::DRMMagic request;
        mp::DRMAuthMagicStatus response;

        mediator.drm_auth_magic(nullptr, &request, &response, null_callback.get());
    }, std::logic_error);

    EXPECT_THROW({
        mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
    }, std::logic_error);
}

TEST_F(ApplicationMediatorTest, can_reconnect_after_disconnect)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());
}

TEST_F(ApplicationMediatorTest, connect_queries_supported_pixel_formats)
{
    using namespace testing;

    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    std::vector<geom::PixelFormat> const pixel_formats{
        geom::PixelFormat::bgr_888,
        geom::PixelFormat::abgr_8888,
        geom::PixelFormat::xbgr_8888
    };

    EXPECT_CALL(*buffer_allocator, supported_pixel_formats())
        .WillOnce(Return(pixel_formats));

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    auto info = connection.display_info();

    ASSERT_EQ(pixel_formats.size(), static_cast<size_t>(info.supported_pixel_format_size()));

    for (size_t i = 0; i < pixel_formats.size(); ++i)
    {
        EXPECT_EQ(pixel_formats[i], static_cast<geom::PixelFormat>(info.supported_pixel_format(i)))
            << "i = " << i;
    }
}
