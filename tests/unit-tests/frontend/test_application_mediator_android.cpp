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

#include "mir/frontend/application_listener.h"
#include "mir/frontend/application_mediator.h"
#include "mir/frontend/resource_cache.h"
#include "mir/frontend/session.h"
#include "mir/frontend/session_store.h"
#include "mir/frontend/surface_organiser.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"

#include <gtest/gtest.h>

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
 * hierarchy.
 *
 * In particular, it would be nice if mf::Session was stubable/mockable.
 */

class StubSurfaceOrganiser : public mf::SurfaceOrganiser
{
 public:
    std::weak_ptr<ms::Surface> create_surface(const ms::SurfaceCreationParameters& /*params*/)
    {
        return std::weak_ptr<ms::Surface>();
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
        return std::make_shared<mf::Session>(organiser, "stub");
    }

    void close_session(std::shared_ptr<mf::Session> const& /*session*/) {}

    void shutdown() {}

    std::shared_ptr<mf::SurfaceOrganiser> organiser;
};

class StubDisplay : public mg::Display
{
 public:
    geom::Rectangle view_area() const { return geom::Rectangle(); }
    void clear() {}
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

struct ApplicationMediatorAndroidTest : public ::testing::Test
{
    ApplicationMediatorAndroidTest()
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
    std::shared_ptr<StubPlatform> const graphics_platform;
    std::shared_ptr<mg::Display> const graphics_display;
    std::shared_ptr<mf::ApplicationListener> const listener;
    std::shared_ptr<mf::ResourceCache> const resource_cache;
    mf::ApplicationMediator mediator;

    std::unique_ptr<google::protobuf::Closure> null_callback;
};

TEST_F(ApplicationMediatorAndroidTest, drm_auth_magic_throws)
{
    mp::ConnectParameters connect_parameters;
    mp::Connection connection;

    mediator.connect(nullptr, &connect_parameters, &connection, null_callback.get());

    mp::DRMMagic magic;
    magic.set_magic(0x10111213);

    EXPECT_THROW({
        mediator.drm_auth_magic(nullptr, &magic, nullptr, null_callback.get());
    }, std::runtime_error);
}
