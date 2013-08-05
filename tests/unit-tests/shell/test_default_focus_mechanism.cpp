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

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/mock_surface_factory.h"
#include "mir_test_doubles/stub_surface.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/stub_surface_builder.h"
#include "mir_test_doubles/stub_surface_controller.h"
#include "mir_test_doubles/stub_input_targeter.h"
#include "mir_test_doubles/mock_input_targeter.h"
#include "mir_test_doubles/mock_display_changer.h"
#include "mir_test_doubles/stub_surface_controller.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace ms = mir::surfaces;
namespace mf = mir::frontend;
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
};

struct DefaultFocusMechanism : public testing::Test
{
    void SetUp()
    {

        controller = std::make_shared<mtd::StubSurfaceController>();
    }
    testing::NiceMock<mtd::MockDisplayChanger> mock_display_changer;
    testing::NiceMock<MockShellSession> app1;
    std::shared_ptr<mtd::StubSurfaceController> controller;
};

}

TEST_F(DefaultFocusMechanism, raises_default_surface)
{
    using namespace ::testing;
    
    mtd::MockSurface mock_surface(std::make_shared<mtd::StubSurfaceBuilder>());
    {
        InSequence seq;
        EXPECT_CALL(app1, default_surface()).Times(1)
            .WillOnce(Return(mt::fake_shared(mock_surface)));
    }

    EXPECT_CALL(mock_surface, raise(Eq(controller))).Times(1);
    mtd::StubInputTargeter targeter;
    msh::DefaultFocusMechanism focus_mechanism(
        mt::fake_shared(targeter), controller, mt::fake_shared(mock_display_changer));
    
    focus_mechanism.set_focus_to(mt::fake_shared(app1));
}

TEST_F(DefaultFocusMechanism, sets_input_focus)
{
    using namespace ::testing;
    
    mtd::MockSurface mock_surface(std::make_shared<mtd::StubSurfaceBuilder>());
    {
        InSequence seq;
        EXPECT_CALL(app1, default_surface()).Times(1)
            .WillOnce(Return(mt::fake_shared(mock_surface)));
        EXPECT_CALL(app1, default_surface()).Times(1)
            .WillOnce(Return(std::shared_ptr<msh::Surface>()));
    }

    mtd::MockInputTargeter targeter;
    
    msh::DefaultFocusMechanism focus_mechanism(
        mt::fake_shared(targeter), controller, mt::fake_shared(mock_display_changer));
    
    Sequence seq;
    EXPECT_CALL(mock_surface, take_input_focus(_))
        .InSequence(seq);
    // When we have no default surface.
    EXPECT_CALL(targeter, focus_cleared())
        .InSequence(seq);
    // When we have no session.
    EXPECT_CALL(targeter, focus_cleared())
        .InSequence(seq);
    
    focus_mechanism.set_focus_to(mt::fake_shared(app1));
    focus_mechanism.set_focus_to(mt::fake_shared(app1));
    focus_mechanism.set_focus_to(std::shared_ptr<msh::Session>());
}
