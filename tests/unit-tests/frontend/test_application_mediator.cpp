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

#include "mir/compositor/buffer_bundle.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/frontend/application_listener.h"
#include "mir/frontend/application_mediator.h"
#include "mir/frontend/resource_cache.h"
#include "mir/frontend/session.h"
#include "mir/frontend/session_store.h"
#include "mir/frontend/surface_organiser.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/surfaces/surface.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;

namespace
{

/*
 * TODO: Fix design so that it's possible to unit-test ApplicationMediator
 * without having to create doubles for classes so deep in its dependency
 * hierarchy, and needing to resort to ugly tricks to get the information
 * we need (e.g. see DestructionRecordingSession below).
 *
 * In particular, it would be nice if both mf::Session and ms::Surface were
 * stubable/mockable.
 */

class DestructionRecordingSession : public mf::Session
{
public:
    DestructionRecordingSession(std::shared_ptr<mf::SurfaceOrganiser> const& surface_organiser)
        : mf::Session{surface_organiser, "Stub"}
    {
        destroyed = false;
    }

    ~DestructionRecordingSession() { destroyed = true; }

    static bool destroyed;
};

bool DestructionRecordingSession::destroyed{true};

class StubBufferBundle : public mc::BufferBundle
{
public:
    std::shared_ptr<mc::GraphicBufferClientResource> secure_client_buffer()
    {
        return std::make_shared<mc::GraphicBufferClientResource>(
            std::make_shared<mc::BufferIPCPackage>(),
            std::shared_ptr<mc::Buffer>(),
            mc::BufferID{0});
    }

    std::shared_ptr<mc::GraphicRegion> lock_back_buffer()
    {
        return std::shared_ptr<mc::GraphicRegion>();
    }

    geom::PixelFormat get_bundle_pixel_format()
    {
        return geom::PixelFormat();
    }

    geom::Size bundle_size()
    {
        return geom::Size();
    }

    void shutdown() {}
};

class StubSurfaceOrganiser : public mf::SurfaceOrganiser
{
 public:
    std::weak_ptr<ms::Surface> create_surface(const ms::SurfaceCreationParameters& /*params*/)
    {
        auto surface = std::make_shared<ms::Surface>("DummySurface",
                                                     std::make_shared<StubBufferBundle>());
        surfaces.push_back(surface);

        return std::weak_ptr<ms::Surface>(surface);
    }

    void destroy_surface(std::weak_ptr<ms::Surface> const& /*surface*/) {}

    void hide_surface(std::weak_ptr<ms::Surface> const& /*surface*/) {}

    void show_surface(std::weak_ptr<ms::Surface> const& /*surface*/) {}

    std::vector<std::shared_ptr<ms::Surface>> surfaces;
};

class StubSessionStore : public mf::SessionStore
{
public:
    StubSessionStore()
        : organiser{std::make_shared<StubSurfaceOrganiser>()}
    {
    }

    std::shared_ptr<mf::Session> open_session(std::string const& /*name*/)
    {
        return std::make_shared<DestructionRecordingSession>(organiser);
    }

    void close_session(std::shared_ptr<mf::Session> const& /*session*/) {}

    void shutdown() {}

    std::shared_ptr<mf::SurfaceOrganiser> organiser;
};

class StubDisplay : public mg::Display
{
 public:
    geom::Rectangle view_area() const { return geom::Rectangle(); }
    void clear() { std::this_thread::yield(); }
    bool post_update() { return true; }
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
        return std::make_shared<StubDisplay>();
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
          graphics_display{std::make_shared<StubDisplay>()},
          listener{std::make_shared<mf::NullApplicationListener>()},
          resource_cache{std::make_shared<mf::ResourceCache>()},
          mediator{session_store, graphics_platform, graphics_display,
                   listener, resource_cache},
          null_callback{google::protobuf::NewPermanentCallback(google::protobuf::DoNothing)}
    {
    }

    std::shared_ptr<mf::SessionStore> const session_store;
    std::shared_ptr<mg::Platform> const graphics_platform;
    std::shared_ptr<mg::Display> const graphics_display;
    std::shared_ptr<mf::ApplicationListener> const listener;
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
    }, std::runtime_error);

    EXPECT_THROW({
        mp::SurfaceId request;
        mp::Buffer response;

        mediator.next_buffer(nullptr, &request, &response, null_callback.get());
    }, std::runtime_error);

    EXPECT_THROW({
        mp::SurfaceId request;

        mediator.release_surface(nullptr, &request, nullptr, null_callback.get());
    }, std::runtime_error);

    EXPECT_THROW({
        mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
    }, std::runtime_error);
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
    }, std::runtime_error);

    EXPECT_THROW({
        mp::SurfaceId request;
        mp::Buffer response;

        mediator.next_buffer(nullptr, &request, &response, null_callback.get());
    }, std::runtime_error);

    EXPECT_THROW({
        mp::SurfaceId request;

        mediator.release_surface(nullptr, &request, nullptr, null_callback.get());
    }, std::runtime_error);

    EXPECT_THROW({
        mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());
    }, std::runtime_error);
}

TEST_F(ApplicationMediatorTest, can_reconnect_after_disconnect)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    mediator.disconnect(nullptr, nullptr, nullptr, null_callback.get());

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());
}
