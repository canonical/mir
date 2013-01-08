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

#include "mir/compositor/buffer_bundle.h"
#include "mir/sessions/session.h"
#include "mir/sessions/surface_creation_parameters.h"
#include "mir/surfaces/surface.h"
#include "mir_test_doubles/mock_buffer_bundle.h"
#include "mir_test/empty_deleter.h"
#include "mir_test_doubles/mock_surface_organiser.h"
#include "mir_test_doubles/null_buffer_bundle.h"

#include "src/surfaces/proxy_surface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace msess = mir::sessions;
namespace ms = mir::surfaces;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

TEST(Session, create_and_destroy_surface)
{
    using namespace ::testing;

    std::shared_ptr<mc::BufferBundle> buffer_bundle(new mtd::NullBufferBundle());
    std::shared_ptr<ms::Surface> dummy_surface(
        new ms::Surface(
            msess::a_surface().name,
            buffer_bundle));

    mtd::MockSurfaceOrganiser organiser;
    msess::Session session(std::shared_ptr<msess::SurfaceOrganiser>(&organiser, mir::EmptyDeleter()), "Foo");
    ON_CALL(organiser, create_surface(_)).WillByDefault(
        Return(std::make_shared<ms::ProxySurface>(dummy_surface)));
    EXPECT_CALL(organiser, create_surface(_));
    EXPECT_CALL(organiser, destroy_surface(_));

    msess::SurfaceCreationParameters params;
    auto surf = session.create_surface(params);

    session.destroy_surface(surf);
}


TEST(Session, session_visbility_propagates_to_surfaces)
{
    using namespace ::testing;

    std::shared_ptr<mc::BufferBundle> buffer_bundle(new mtd::NullBufferBundle());
    std::shared_ptr<ms::Surface> dummy_surface(
        new ms::Surface(
            msess::a_surface().name,
            buffer_bundle));

    mtd::MockSurfaceOrganiser organiser;
    msess::Session app_session(std::shared_ptr<msess::SurfaceOrganiser>(&organiser, mir::EmptyDeleter()), "Foo");
    ON_CALL(organiser, create_surface(_)).WillByDefault(Return(
        std::make_shared<ms::ProxySurface>(dummy_surface)));
    EXPECT_CALL(organiser, create_surface(_));
    EXPECT_CALL(organiser, destroy_surface(_));

    {
        InSequence seq;
        EXPECT_CALL(organiser, hide_surface(_)).Times(1);
        EXPECT_CALL(organiser, show_surface(_)).Times(1);
    }

    msess::SurfaceCreationParameters params;
    auto surf = app_session.create_surface(params);

    app_session.hide();
    app_session.show();

    app_session.destroy_surface(surf);
}
