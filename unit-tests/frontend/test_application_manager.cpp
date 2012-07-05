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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/compositor/buffer_bundle.h"
#include "mir/compositor/buffer_texture_binder.h"
#include "mir/frontend/application_manager.h"
#include "mir/surfaces/application_surface_organiser.h"
#include "mir/surfaces/surface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::surfaces;

namespace
{

struct MockBufferTextureBinder : mc::BufferTextureBinder
{
    MOCK_METHOD0(lock_back_buffer, void());
    MOCK_METHOD0(unlock_back_buffer, void());

    MOCK_METHOD0(back_buffer, std::shared_ptr<mc::Buffer>());
};

struct MockApplicationSurfaceOrganiser : public ms::ApplicationSurfaceOrganiser
{
    MOCK_METHOD1(create_surface, std::weak_ptr<ms::Surface>(const ms::SurfaceCreationParameters&));
    MOCK_METHOD1(destroy_surface, void(std::weak_ptr<ms::Surface> surface));
};

}

TEST(ApplicationManagerDeathTest, class_invariants_not_satisfied_triggers_assertion)
{
    EXPECT_EXIT(
        mir::frontend::ApplicationManager app(nullptr),
        ::testing::KilledBySignal(SIGABRT),
        ".*");
}

TEST(ApplicationManager, create_and_destroy_surface)
{
    using namespace ::testing;

    std::shared_ptr<mc::BufferTextureBinder> buffer_texture_binder(
        new MockBufferTextureBinder());
    std::shared_ptr<ms::Surface> dummy_surface(
        new ms::Surface(
            ms::a_surface(),
            buffer_texture_binder));
    
    MockApplicationSurfaceOrganiser organizer;
    mf::ApplicationManager app_manager(&organizer);
    ON_CALL(organizer, create_surface(_)).WillByDefault(Return(dummy_surface));
    EXPECT_CALL(organizer, create_surface(_));
    EXPECT_CALL(organizer, destroy_surface(_));

    ms::SurfaceCreationParameters params;
    std::weak_ptr<ms::Surface> surface{app_manager.create_surface(params)};
    ASSERT_TRUE(surface.lock().get() != nullptr);
    
    app_manager.destroy_surface(surface);
}

