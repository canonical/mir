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
#include "mir/shell/session_store.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/graphics/viewable_area.h"

#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtc = mt::cucumber;

namespace
{

struct MockServerConfiguration : public mir::ServerConfiguration
{
    MOCK_METHOD0(the_communicator, std::shared_ptr<mf::Communicator>());
    MOCK_METHOD0(the_session_store, std::shared_ptr<msh::SessionStore>());
    MOCK_METHOD0(the_input_manager, std::shared_ptr<mi::InputManager>());
    MOCK_METHOD0(the_display, std::shared_ptr<mg::Display>());
    MOCK_METHOD0(the_drawer, std::shared_ptr<mc::Drawer>());
};

struct MockSessionStore : public msh::SessionStore
{
    MOCK_METHOD1(open_session, std::shared_ptr<msh::Session>(std::string const&));
    MOCK_METHOD1(close_session, void(std::shared_ptr<msh::Session> const&));

    MOCK_METHOD2(tag_session_with_lightdm_id, void(std::shared_ptr<msh::Session> const&, int));
    MOCK_METHOD1(focus_session_with_lightdm_id, void(int));

    MOCK_METHOD0(shutdown, void());
};

struct MockSession : public msh::Session
{
    MOCK_METHOD1(create_surface, msh::SurfaceId(msh::SurfaceCreationParameters const&));
    MOCK_METHOD1(destroy_surface, void(msh::SurfaceId));
    MOCK_CONST_METHOD1(get_surface, std::shared_ptr<msh::Surface>(msh::SurfaceId));
    
    MOCK_METHOD0(name, std::string());
    MOCK_METHOD0(shutdown, void());
    
    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());
};

struct MockSurface : public msh::Surface
{
    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());
    MOCK_METHOD0(destroy, void());
    MOCK_METHOD0(shutdown, void());
    MOCK_METHOD0(advance_client_buffer, void());

    MOCK_CONST_METHOD0(size, mir::geometry::Size ());
    MOCK_CONST_METHOD0(pixel_format, mir::geometry::PixelFormat ());
    MOCK_CONST_METHOD0(client_buffer, std::shared_ptr<mc::Buffer> ());
};

MATCHER_P(NamedWindowWithNoGeometry, name, "")
{
    if (arg.name != name)
        return false;

    if (arg.size.width.as_uint32_t() != 0)
        return false;
    if (arg.size.height.as_uint32_t() != 0)
        return false;

    return true;
}

MATCHER_P2(NamedWindowWithGeometry, name, geometry, "")
{
    if (arg.name != name)
        return false;
    if (arg.size != geometry)
        return false;
    return true;
}

struct SessionManagementContextSetup : public testing::Test
{
    void SetUp()
    {
        using namespace ::testing;

        EXPECT_CALL(server_configuration, the_session_store()).Times(1)
            .WillOnce(Return(mt::fake_shared<msh::SessionStore>(session_store)));
        ctx = std::make_shared<mtc::SessionManagementContext>(server_configuration);
    }
    MockServerConfiguration server_configuration;
    MockSessionStore session_store;
    std::shared_ptr<mtc::SessionManagementContext> ctx;

    static msh::SurfaceId const test_surface_id;
    static std::string const test_window_name;
    static geom::Size const test_window_size;
};

struct SessionManagementContextViewAreaSetup : public SessionManagementContextSetup
{
    void SetUp()
    {
        using namespace ::testing;

        EXPECT_CALL(server_configuration, the_session_store()).Times(1)
            .WillOnce(Return(mt::fake_shared<msh::SessionStore>(session_store)));
        ctx = std::make_shared<mtc::SessionManagementContext>(server_configuration);
        viewable_area = ctx->get_view_area();
    }

    std::shared_ptr<mg::ViewableArea> viewable_area;
};

msh::SurfaceId const SessionManagementContextSetup::test_surface_id{1};
std::string const SessionManagementContextSetup::test_window_name{"John"};
geom::Size const SessionManagementContextSetup::test_window_size{geom::Width{100},
                                                                 geom::Height{100}};
} // namespace

TEST(SessionManagementContext, constructs_session_store_from_server_configuration)
{
    using namespace ::testing;

    MockServerConfiguration server_configuration;
    MockSessionStore session_store;
    
    EXPECT_CALL(server_configuration, the_session_store()).Times(1)
        .WillOnce(Return(mt::fake_shared<msh::SessionStore>(session_store)));

    mtc::SessionManagementContext ctx(server_configuration);
}

TEST_F(SessionManagementContextSetup, open_window_consuming_creates_surface_with_no_geometry)
{
    using namespace ::testing;

    MockSession session;

    EXPECT_CALL(session_store, open_session(test_window_name)).Times(1)
        .WillOnce(Return(mt::fake_shared<msh::Session>(session)));

    // As consuming mode is the default, omiting geometry is sufficient to request it.
    EXPECT_CALL(session, create_surface(NamedWindowWithNoGeometry(test_window_name))).Times(1)
        .WillOnce(Return(test_surface_id));

    EXPECT_TRUE(ctx->open_window_consuming(test_window_name));
}

TEST_F(SessionManagementContextSetup, open_window_with_size_creates_surface_with_size)
{
    using namespace ::testing;
    
    MockSession session;

    EXPECT_CALL(session_store, open_session(test_window_name)).Times(1)
        .WillOnce(Return(mt::fake_shared<msh::Session>(session)));

    EXPECT_CALL(session, create_surface(NamedWindowWithGeometry(test_window_name, test_window_size))).Times(1)
        .WillOnce(Return(test_surface_id));

    EXPECT_TRUE(ctx->open_window_with_size(test_window_name, test_window_size));
}

TEST_F(SessionManagementContextSetup, get_window_size_queries_surface)
{
    using namespace ::testing;

    MockSession session;
    MockSurface surface;

    EXPECT_CALL(session_store, open_session(test_window_name)).Times(1)
        .WillOnce(Return(mt::fake_shared<msh::Session>(session)));

    EXPECT_CALL(session, create_surface(NamedWindowWithGeometry(test_window_name, test_window_size))).Times(1)
        .WillOnce(Return(test_surface_id));

    EXPECT_TRUE(ctx->open_window_with_size(test_window_name, test_window_size));
    
    EXPECT_CALL(session, get_surface(test_surface_id)).Times(1)
        .WillOnce(Return(mt::fake_shared<msh::Surface>(surface)));
    EXPECT_CALL(surface, size()).Times(1).WillOnce(Return(test_window_size));
    
    EXPECT_EQ(test_window_size, ctx->get_window_size(test_window_name));
}

TEST_F(SessionManagementContextViewAreaSetup, default_view_area_is_1600_by_1400)
{
    using namespace ::testing;

    static const geom::Size default_view_size = geom::Size{geom::Width{1600},
                                                           geom::Height{1400}};

    EXPECT_EQ(default_view_size, viewable_area->view_area().size);
}


TEST_F(SessionManagementContextViewAreaSetup, set_view_area_updates_viewable_area)
{
    using namespace ::testing;

    static const geom::Rectangle new_view_area = geom::Rectangle{geom::Point(),
                                                                  geom::Size{geom::Width{100},
                                                                             geom::Height{100}}};

    ctx->set_view_area(new_view_area);
    EXPECT_EQ(new_view_area, viewable_area->view_area());
}
