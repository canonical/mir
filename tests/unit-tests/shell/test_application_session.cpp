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
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#include "mir/shell/application_session.h"
#include "mir/compositor/buffer.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface_factory.h"

#include "src/surfaces/proxy_surface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace ms = mir::surfaces;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
class MockSurface : public msh::Surface
{
public:
    MockSurface(std::weak_ptr<ms::Surface> const& surface) :
        impl(surface)
    {
        using namespace testing;

        ON_CALL(*this, hide()).WillByDefault(Invoke(&impl, &ms::BasicProxySurface::hide));
        ON_CALL(*this, show()).WillByDefault(Invoke(&impl, &ms::BasicProxySurface::show));
        ON_CALL(*this, destroy()).WillByDefault(Invoke(&impl, &ms::BasicProxySurface::destroy));
        ON_CALL(*this, shutdown()).WillByDefault(Invoke(&impl, &ms::BasicProxySurface::shutdown));
        ON_CALL(*this, advance_client_buffer()).WillByDefault(Invoke(&impl, &ms::BasicProxySurface::advance_client_buffer));

        ON_CALL(*this, size()).WillByDefault(Invoke(&impl, &ms::BasicProxySurface::size));
        ON_CALL(*this, pixel_format()).WillByDefault(Invoke(&impl, &ms::BasicProxySurface::pixel_format));
        ON_CALL(*this, client_buffer()).WillByDefault(Invoke(&impl, &ms::BasicProxySurface::client_buffer));
    }

    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());
    MOCK_METHOD0(destroy, void());
    MOCK_METHOD0(shutdown, void());
    MOCK_METHOD0(advance_client_buffer, void());

    MOCK_CONST_METHOD0(size, mir::geometry::Size ());
    MOCK_CONST_METHOD0(pixel_format, mir::geometry::PixelFormat ());
    MOCK_CONST_METHOD0(client_buffer, std::shared_ptr<mc::Buffer> ());

private:
    ms::BasicProxySurface impl;
};
}

TEST(ApplicationSession, create_and_destroy_surface)
{
    using namespace ::testing;

    auto const mock_surface = std::make_shared<MockSurface>(std::shared_ptr<ms::Surface>());
    mtd::MockSurfaceFactory surface_factory;
    ON_CALL(surface_factory, create_surface(_)).WillByDefault(Return(mock_surface));

    EXPECT_CALL(surface_factory, create_surface(_));
    EXPECT_CALL(*mock_surface, destroy());

    msh::ApplicationSession session(mt::fake_shared(surface_factory), "Foo");

    msh::SurfaceCreationParameters params;
    auto surf = session.create_surface(params);

    session.destroy_surface(surf);
}


TEST(ApplicationSession, session_visbility_propagates_to_surfaces)
{
    using namespace ::testing;

    auto const mock_surface = std::make_shared<MockSurface>(std::shared_ptr<ms::Surface>());
    mtd::MockSurfaceFactory surface_factory;
    ON_CALL(surface_factory, create_surface(_)).WillByDefault(Return(mock_surface));

    msh::ApplicationSession app_session(mt::fake_shared(surface_factory), "Foo");

    EXPECT_CALL(surface_factory, create_surface(_));

    {
        InSequence seq;
        EXPECT_CALL(*mock_surface, hide()).Times(1);
        EXPECT_CALL(*mock_surface, show()).Times(1);
        EXPECT_CALL(*mock_surface, destroy()).Times(1);
    }

    msh::SurfaceCreationParameters params;
    auto surf = app_session.create_surface(params);

    app_session.hide();
    app_session.show();

    app_session.destroy_surface(surf);
}

TEST(Session, get_invalid_surface_throw_behavior)
{
    using namespace ::testing;

    mtd::MockSurfaceFactory surface_factory;
    msh::ApplicationSession app_session(mt::fake_shared(surface_factory), "Foo");
    msh::SurfaceId invalid_surface_id = msh::SurfaceId{1};

    EXPECT_THROW({
            app_session.get_surface(invalid_surface_id);
    }, std::runtime_error);
}

TEST(Session, destroy_invalid_surface_throw_behavior)
{
    using namespace ::testing;

    mtd::MockSurfaceFactory surface_factory;
    msh::ApplicationSession app_session(mt::fake_shared(surface_factory), "Foo");
    msh::SurfaceId invalid_surface_id = msh::SurfaceId{1};

    EXPECT_THROW({
            app_session.destroy_surface(invalid_surface_id);
    }, std::runtime_error);
}
