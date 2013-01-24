/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "session_management_context.h"
#include "mir/server_configuration.h"
#include "mir/sessions/session_store.h"
#include "mir/sessions/session.h"
#include "mir/sessions/surface.h"
#include "mir/sessions/surface_creation_parameters.h"
#include "mir/graphics/viewable_area.h"

#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mo = mir::options;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace msess = mir::sessions;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtc = mt::cucumber;

namespace
{

struct MockServerConfiguration : public mir::ServerConfiguration
{
    MOCK_METHOD0(make_options, std::shared_ptr<mo::Option>());
    MOCK_METHOD0(make_graphics_platform, std::shared_ptr<mg::Platform>());
    MOCK_METHOD0(make_buffer_initializer, std::shared_ptr<mg::BufferInitializer>());
    MOCK_METHOD1(make_buffer_allocation_strategy, std::shared_ptr<mc::BufferAllocationStrategy>(std::shared_ptr<mc::GraphicBufferAllocator> const&));
    MOCK_METHOD1(make_renderer, std::shared_ptr<mg::Renderer>(std::shared_ptr<mg::Display> const&));
    MOCK_METHOD3(make_communicator, std::shared_ptr<mf::Communicator>(std::shared_ptr<msess::SessionStore> const&,
                                                     std::shared_ptr<mg::Display> const&,
                                                     std::shared_ptr<mc::GraphicBufferAllocator> const&));
    MOCK_METHOD2(make_session_store, std::shared_ptr<msess::SessionStore>(std::shared_ptr<msess::SurfaceFactory> const&,
                                                                          std::shared_ptr<mg::ViewableArea> const&));
    MOCK_METHOD2(make_input_manager, std::shared_ptr<mi::InputManager>(std::initializer_list<std::shared_ptr<mi::EventFilter> const> const&, std::shared_ptr<mg::ViewableArea> const&));
};

struct MockSessionStore : public msess::SessionStore
{
    MOCK_METHOD1(open_session, std::shared_ptr<msess::Session>(std::string const&));
    MOCK_METHOD1(close_session, void(std::shared_ptr<msess::Session> const&));

    MOCK_METHOD2(tag_session_with_lightdm_id, void(std::shared_ptr<msess::Session> const&, int));
    MOCK_METHOD1(focus_session_with_lightdm_id, void(int));

    MOCK_METHOD0(shutdown, void());
};

struct MockSurface : public msess::Surface
{
    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());
    MOCK_METHOD0(destroy, void());
    MOCK_METHOD0(shutdown, void());
    MOCK_METHOD0(advance_client_buffer, void());

    MOCK_CONST_METHOD0(size, mir::geometry::Size ());
    MOCK_CONST_METHOD0(pixel_format, mir::geometry::PixelFormat ());
    MOCK_CONST_METHOD0(client_buffer_resource, std::shared_ptr<mc::GraphicBufferClientResource> ());
};

struct MockSession : public msess::Session
{
    MOCK_METHOD1(create_surface, msess::SurfaceId(msess::SurfaceCreationParameters const&));
    MOCK_METHOD1(destroy_surface, void(msess::SurfaceId));
    MOCK_CONST_METHOD1(get_surface, std::shared_ptr<msess::Surface>(msess::SurfaceId));
    
    MOCK_METHOD0(name, std::string());
    MOCK_METHOD0(shutdown, void());
    
    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());
};

MATCHER_P(NamedParametersWithNoGeometry, name, "")
{
    if (arg.name != name)
        return false;
    
    if (arg.size.width.as_uint32_t() != 0)
        return false;
    if (arg.size.height.as_uint32_t() != 0)
        return false;
    
    return true;
}

MATCHER_P2(NamedParametersWithGeometry, name, geometry, "")
{
    if (arg.name != name)
        return false;
    if (arg.size != geometry)
        return false;
    return true;
}

} // namespace

TEST(SessionManagementContext, constructs_session_store_from_server_configuration)
{
    using namespace ::testing;
    MockServerConfiguration server_configuration;
    MockSessionStore session_store;
    
    EXPECT_CALL(server_configuration, make_session_store(_, _)).Times(1)
        .WillOnce(Return(mt::fake_shared<msess::SessionStore>(session_store)));
    
    mtc::SessionManagementContext ctx(mt::fake_shared<mir::ServerConfiguration>(server_configuration));
}

TEST(SessionManagementContext, open_window_consuming_creates_surface_with_no_geometry)
{
    using namespace ::testing;
    MockServerConfiguration server_configuration;
    MockSessionStore session_store;
    MockSession session;

    msess::SurfaceId const testing_surface_id{1};
    std::string const testing_window_name = "John";
    
    EXPECT_CALL(server_configuration, make_session_store(_, _)).Times(1)
        .WillOnce(Return(mt::fake_shared<msess::SessionStore>(session_store)));
    
    EXPECT_CALL(session_store, open_session(testing_window_name)).Times(1)
        .WillOnce(Return(mt::fake_shared<msess::Session>(session)));

    // To request consuming mode it is sufficient to omit geometry in the creation request.
    EXPECT_CALL(session, create_surface(NamedParametersWithNoGeometry(testing_window_name))).Times(1)
        .WillOnce(Return(testing_surface_id));
    
    mtc::SessionManagementContext ctx(mt::fake_shared<mir::ServerConfiguration>(server_configuration));
    
    EXPECT_TRUE(ctx.open_window_consuming(testing_window_name));
}

TEST(SessionManagementContext, open_window_with_size_creates_surface_with_size)
{
    using namespace ::testing;
    MockServerConfiguration server_configuration;
    MockSessionStore session_store;
    MockSession session;

    msess::SurfaceId const testing_surface_id{1};
    std::string const testing_window_name = "John";
    geom::Size const testing_window_size = geom::Size{geom::Width{100}, geom::Height{100}};
    
    EXPECT_CALL(server_configuration, make_session_store(_, _)).Times(1)
        .WillOnce(Return(mt::fake_shared<msess::SessionStore>(session_store)));
    
    EXPECT_CALL(session_store, open_session(testing_window_name)).Times(1)
        .WillOnce(Return(mt::fake_shared<msess::Session>(session)));

    EXPECT_CALL(session, create_surface(NamedParametersWithGeometry(testing_window_name, testing_window_size))).Times(1)
        .WillOnce(Return(testing_surface_id));
    
    mtc::SessionManagementContext ctx(mt::fake_shared<mir::ServerConfiguration>(server_configuration));
    
    EXPECT_TRUE(ctx.open_window_with_size(testing_window_name, testing_window_size));
}

TEST(SessionManagementContext, get_window_size_queries_surface)
{
    using namespace ::testing;
    MockServerConfiguration server_configuration;
    MockSessionStore session_store;
    MockSession session;
    MockSurface surface;

    msess::SurfaceId const testing_surface_id{1};
    std::string const testing_window_name = "John";
    geom::Size const testing_window_size = geom::Size{geom::Width{100}, geom::Height{100}};
    
    EXPECT_CALL(server_configuration, make_session_store(_, _)).Times(1)
        .WillOnce(Return(mt::fake_shared<msess::SessionStore>(session_store)));
    
    EXPECT_CALL(session_store, open_session(testing_window_name)).Times(1)
        .WillOnce(Return(mt::fake_shared<msess::Session>(session)));

    EXPECT_CALL(session, create_surface(NamedParametersWithGeometry(testing_window_name, testing_window_size))).Times(1)
        .WillOnce(Return(testing_surface_id));
    
    mtc::SessionManagementContext ctx(mt::fake_shared<mir::ServerConfiguration>(server_configuration));
    
    EXPECT_TRUE(ctx.open_window_with_size(testing_window_name, testing_window_size));
    
    EXPECT_CALL(session, get_surface(testing_surface_id)).Times(1)
        .WillOnce(Return(mt::fake_shared<msess::Surface>(surface)));
    EXPECT_CALL(surface, size()).Times(1).WillOnce(Return(testing_window_size));
    
    EXPECT_EQ(testing_window_size, ctx.get_window_size(testing_window_name));
}

TEST(SessionManagementContext, default_view_area_is_1600_by_1400)
{
    using namespace ::testing;

    static const geom::Width default_view_width = geom::Width{1600};
    static const geom::Height default_view_height = geom::Height{1400};
    
    static const geom::Size default_view_size = geom::Size{default_view_width,
                                                       default_view_height};

    MockServerConfiguration server_configuration;
    MockSessionStore session_store;
    std::shared_ptr<mg::ViewableArea> viewable_area;

    EXPECT_CALL(server_configuration, make_session_store(_, _)).Times(1)
        .WillOnce(DoAll(SaveArg<1>(&viewable_area), Return(mt::fake_shared<msess::SessionStore>(session_store))));

    mtc::SessionManagementContext ctx(mt::fake_shared<mir::ServerConfiguration>(server_configuration));
    
    EXPECT_EQ(default_view_size, viewable_area->view_area().size);
}


TEST(SessionManagementContext, set_view_area_updates_viewable_area)
{
    using namespace ::testing;
    MockServerConfiguration server_configuration;
    MockSessionStore session_store;
    std::shared_ptr<mg::ViewableArea> viewable_area;
    
    static const geom::Rectangle updated_region = geom::Rectangle{geom::Point(),
                                                                  geom::Size{geom::Width{100},
                                                                             geom::Height{100}}};
    
    EXPECT_CALL(server_configuration, make_session_store(_, _)).Times(1)
        .WillOnce(DoAll(SaveArg<1>(&viewable_area), Return(mt::fake_shared<msess::SessionStore>(session_store))));
    
    mtc::SessionManagementContext ctx(mt::fake_shared<mir::ServerConfiguration>(server_configuration));
    ctx.set_view_area(updated_region);
    
    EXPECT_EQ(updated_region, viewable_area->view_area());
}
