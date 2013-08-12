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
#include "mir/shell/registration_order_focus_sequence.h"
#include "mir/shell/default_focus_mechanism.h"
#include "mir/shell/session.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/surfaces/surface.h"
#include "mir/graphics/display_configuration.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/mock_surface_factory.h"
#include "mir_test_doubles/mock_shell_session.h"
#include "mir_test_doubles/stub_surface.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/stub_surface_builder.h"
#include "mir_test_doubles/stub_surface_controller.h"
#include "mir_test_doubles/stub_input_targeter.h"
#include "mir_test_doubles/mock_input_targeter.h"
#include "mir_test_doubles/mock_session_listener.h"
#include "mir_test_doubles/stub_surface_controller.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace ms = mir::surfaces;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
struct MockShellSession : public msh::Session
{
    MOCK_METHOD1(create_surface, mf::SurfaceId(msh::SurfaceCreationParameters const&));
    MOCK_METHOD1(destroy_surface, void(mf::SurfaceId));
    MOCK_CONST_METHOD1(get_surface, std::shared_ptr<mf::Surface>(mf::SurfaceId));

    MOCK_METHOD1(take_snapshot, void(msh::SnapshotCallback const&));
    MOCK_CONST_METHOD0(default_surface, std::shared_ptr<msh::Surface>());

    MOCK_CONST_METHOD0(name, std::string());
    MOCK_METHOD0(force_requests_to_complete, void());

    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());
    MOCK_METHOD3(configure_surface, int(mf::SurfaceId, MirSurfaceAttrib, int));
    MOCK_METHOD1(send_display_config, void(mg::DisplayConfiguration const&));
};

struct MockFocusSequence : public msh::FocusSequence
{
    MOCK_CONST_METHOD1(successor_of, std::shared_ptr<msh::Session>(std::shared_ptr<msh::Session> const&));
    MOCK_CONST_METHOD1(predecessor_of, std::shared_ptr<msh::Session>(std::shared_ptr<msh::Session> const&));
    MOCK_CONST_METHOD0(default_focus, std::shared_ptr<msh::Session>());
};

struct DefaultFocusMechanism : public testing::Test
{
    void SetUp()
    {
        controller = std::make_shared<mtd::StubSurfaceController>();

        //TODO: extremely kludgy that a mock needs to be constructed this way 
        mock_surface1 = std::make_shared<mtd::MockSurface>(&app1, std::make_shared<mtd::StubSurfaceBuilder>());
        mock_surface2 = std::make_shared<mtd::MockSurface>(&app2, std::make_shared<mtd::StubSurfaceBuilder>());
        ON_CALL(app1, default_surface())
            .WillByDefault(testing::Return(mock_surface1));
        ON_CALL(app2, default_surface())
            .WillByDefault(testing::Return(mock_surface2));
    }

    std::shared_ptr<mtd::MockSurface> mock_surface1, mock_surface2;
    testing::NiceMock<MockShellSession> app1;
    testing::NiceMock<MockShellSession> app2;
    testing::NiceMock<mtd::MockSessionListener> mock_listener;
    testing::NiceMock<MockFocusSequence> focus_sequence;
    std::shared_ptr<msh::SurfaceController> controller;

    mtd::StubInputTargeter targeter;
};

}

TEST_F(DefaultFocusMechanism, raises_default_surface)
{
    using namespace ::testing;
    
    EXPECT_CALL(mock_listener, focused(_))
        .Times(1);
    EXPECT_CALL(*mock_surface1,
         raise(Eq(controller))).Times(1);

    msh::DefaultFocusMechanism focus_mechanism(mt::fake_shared(focus_sequence),
        mt::fake_shared(targeter), controller, mt::fake_shared(mock_listener));
    
    focus_mechanism.surface_created_for(mt::fake_shared(app1));
}

TEST_F(DefaultFocusMechanism, open_and_close)
{
    using namespace ::testing;

    Sequence seq; 
    EXPECT_CALL(mock_listener, focused(_))
        .InSequence(seq);

    EXPECT_CALL(focus_sequence, successor_of(_))
        .InSequence(seq)
        .WillOnce(Throw(std::logic_error("")));
    EXPECT_CALL(mock_listener, unfocused())
        .InSequence(seq);

    msh::DefaultFocusMechanism focus_mechanism(mt::fake_shared(focus_sequence),
        mt::fake_shared(targeter), controller, mt::fake_shared(mock_listener));

    auto app = mt::fake_shared(app1); 
    focus_mechanism.session_opened(app);
    focus_mechanism.session_closed(app);
}

TEST_F(DefaultFocusMechanism, unfocus_only_on_close_active_session)
{
    using namespace ::testing;

    EXPECT_CALL(mock_listener, focused(_))
        .Times(2);  
    EXPECT_CALL(mock_listener, unfocused())
        .Times(0);

    msh::DefaultFocusMechanism focus_mechanism(mt::fake_shared(focus_sequence),
        mt::fake_shared(targeter), controller, mt::fake_shared(mock_listener));
    
    focus_mechanism.session_opened(mt::fake_shared(app1));
    focus_mechanism.session_opened(mt::fake_shared(app2));
    focus_mechanism.session_closed(mt::fake_shared(app1));
}

TEST_F(DefaultFocusMechanism, sets_input_focus)
{
    using namespace ::testing;
    
    EXPECT_CALL(*mock_surface1, take_input_focus(_)).Times(1);
    
    msh::DefaultFocusMechanism focus_mechanism(mt::fake_shared(focus_sequence), mt::fake_shared(targeter), controller,
        mt::fake_shared(mock_listener));
    
    focus_mechanism.surface_created_for(mt::fake_shared(app1));
}
