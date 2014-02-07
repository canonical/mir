/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#include "src/server/scene/application_session.h"
#include "mir/graphics/buffer.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/shell/null_session_listener.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface_factory.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/mock_session_listener.h"
#include "mir_test_doubles/stub_surface_builder.h"
#include "mir_test_doubles/stub_surface_ranker.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test_doubles/null_snapshot_strategy.h"
#include "mir_test_doubles/null_event_sink.h"

#include "mir/shell/surface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
static std::shared_ptr<mtd::MockSurface> make_mock_surface()
{
    return std::make_shared<mtd::MockSurface>(std::make_shared<mtd::StubSurfaceBuilder>());
}
}

TEST(ApplicationSession, create_and_destroy_surface)
{
    using namespace ::testing;

    auto mock_surface = make_mock_surface();

    mtd::NullEventSink sender;
    mtd::MockSurfaceFactory surface_factory;

    EXPECT_CALL(surface_factory, create_surface(_, _, _, _))
        .WillOnce(Return(mock_surface));

    mtd::MockSessionListener listener;
    EXPECT_CALL(listener, surface_created(_, _))
        .Times(1);
    EXPECT_CALL(listener, destroying_surface(_, _))
        .Times(1);

    ms::ApplicationSession session(
        mt::fake_shared(surface_factory),
        __LINE__,
        "Foo",
        std::make_shared<mtd::NullSnapshotStrategy>(),
        mt::fake_shared(listener),
        mt::fake_shared(sender));

    msh::SurfaceCreationParameters params;
    auto surf = session.create_surface(params);

    session.destroy_surface(surf);
}

TEST(ApplicationSession, listener_notified_of_surface_destruction_on_session_destruction)
{
    using namespace ::testing;

    auto mock_surface = make_mock_surface();

    mtd::NullEventSink sender;
    mtd::MockSurfaceFactory surface_factory;
    ON_CALL(surface_factory, create_surface(_,_,_,_)).WillByDefault(Return(mock_surface));

    EXPECT_CALL(surface_factory, create_surface(_, _, _, _));

    mtd::MockSessionListener listener;
    EXPECT_CALL(listener, surface_created(_, _)).Times(1);
    EXPECT_CALL(listener, destroying_surface(_, _)).Times(1);

    {
        ms::ApplicationSession session(
            mt::fake_shared(surface_factory),
            __LINE__,
            "Foo",
            std::make_shared<mtd::NullSnapshotStrategy>(),
            mt::fake_shared(listener),
            mt::fake_shared(sender));

        msh::SurfaceCreationParameters params;
        session.create_surface(params);
    }
}

TEST(ApplicationSession, default_surface_is_first_surface)
{
    using namespace ::testing;

    mtd::NullEventSink sender;
    mtd::MockSurfaceFactory surface_factory;

    {
        InSequence seq;
        EXPECT_CALL(surface_factory, create_surface(_, _, _, _)).Times(1)
            .WillOnce(Return(make_mock_surface()));
        EXPECT_CALL(surface_factory, create_surface(_, _, _, _)).Times(1)
            .WillOnce(Return(make_mock_surface()));
        EXPECT_CALL(surface_factory, create_surface(_, _, _, _)).Times(1)
            .WillOnce(Return(make_mock_surface()));
    }

    ms::ApplicationSession app_session(
        mt::fake_shared(surface_factory),
        __LINE__,
        "Foo",
        std::make_shared<mtd::NullSnapshotStrategy>(),
        std::make_shared<msh::NullSessionListener>(),
        mt::fake_shared(sender));


    msh::SurfaceCreationParameters params;
    auto id1 = app_session.create_surface(params);
    auto id2 = app_session.create_surface(params);
    auto id3 = app_session.create_surface(params);

    auto default_surf = app_session.default_surface();
    EXPECT_EQ(app_session.get_surface(id1), default_surf);
    app_session.destroy_surface(id1);

    default_surf = app_session.default_surface();
    EXPECT_EQ(app_session.get_surface(id2), default_surf);
    app_session.destroy_surface(id2);

    default_surf = app_session.default_surface();
    EXPECT_EQ(app_session.get_surface(id3), default_surf);
    app_session.destroy_surface(id3);
}

TEST(ApplicationSession, session_visbility_propagates_to_surfaces)
{
    using namespace ::testing;

    mtd::NullEventSink sender;
    auto mock_surface = make_mock_surface();

    mtd::MockSurfaceFactory surface_factory;
    ON_CALL(surface_factory, create_surface(_, _, _, _)).WillByDefault(Return(mock_surface));

    ms::ApplicationSession app_session(
        mt::fake_shared(surface_factory),
        __LINE__,
        "Foo",
        std::make_shared<mtd::NullSnapshotStrategy>(),
        std::make_shared<msh::NullSessionListener>(),
        mt::fake_shared(sender));

    EXPECT_CALL(surface_factory, create_surface(_, _, _, _));

    {
        InSequence seq;
        EXPECT_CALL(*mock_surface, hide()).Times(1);
        EXPECT_CALL(*mock_surface, show()).Times(1);
    }

    msh::SurfaceCreationParameters params;
    auto surf = app_session.create_surface(params);

    app_session.hide();
    app_session.show();

    app_session.destroy_surface(surf);
}

TEST(ApplicationSession, get_invalid_surface_throw_behavior)
{
    using namespace ::testing;

    mtd::NullEventSink sender;
    mtd::MockSurfaceFactory surface_factory;
    ms::ApplicationSession app_session(
        mt::fake_shared(surface_factory),
        __LINE__,
        "Foo",
        std::make_shared<mtd::NullSnapshotStrategy>(),
        std::make_shared<msh::NullSessionListener>(),
        mt::fake_shared(sender));

    mf::SurfaceId invalid_surface_id(1);

    EXPECT_THROW({
            app_session.get_surface(invalid_surface_id);
    }, std::runtime_error);
}

TEST(ApplicationSession, destroy_invalid_surface_throw_behavior)
{
    using namespace ::testing;

    mtd::NullEventSink sender;
    mtd::MockSurfaceFactory surface_factory;
    ms::ApplicationSession app_session(
        mt::fake_shared(surface_factory),
        __LINE__,
        "Foo",
        std::make_shared<mtd::NullSnapshotStrategy>(),
        std::make_shared<msh::NullSessionListener>(),
        mt::fake_shared(sender));

    mf::SurfaceId invalid_surface_id(1);

    EXPECT_THROW({
            app_session.destroy_surface(invalid_surface_id);
    }, std::runtime_error);
}

TEST(ApplicationSession, uses_snapshot_strategy)
{
    using namespace ::testing;

    class MockSnapshotStrategy : public ms::SnapshotStrategy
    {
    public:
        ~MockSnapshotStrategy() noexcept {}

        MOCK_METHOD2(take_snapshot_of,
                    void(std::shared_ptr<msh::SurfaceBufferAccess> const&,
                         msh::SnapshotCallback const&));
    };

    auto snapshot_strategy = std::make_shared<MockSnapshotStrategy>();
    mtd::NullEventSink sender;
    mtd::MockSurfaceFactory surface_factory;
    ms::ApplicationSession app_session(
        mt::fake_shared(surface_factory),
        __LINE__,
        "Foo",
        snapshot_strategy,
        std::make_shared<msh::NullSessionListener>(),
        mt::fake_shared(sender));

    EXPECT_CALL(*snapshot_strategy, take_snapshot_of(_,_));

    app_session.take_snapshot(msh::SnapshotCallback());
}

namespace
{
class MockEventSink : public mf::EventSink
{
public:
    MOCK_METHOD1(handle_event, void(MirEvent const&));
    MOCK_METHOD1(handle_lifecycle_event, void(MirLifecycleState));
    MOCK_METHOD1(handle_display_config_change, void(mir::graphics::DisplayConfiguration const&));
};
}
TEST(ApplicationSession, display_config_sender)
{
    using namespace ::testing;

    mtd::StubDisplayConfig stub_config;
    mtd::MockSurfaceFactory surface_factory;
    MockEventSink sender;

    EXPECT_CALL(sender, handle_display_config_change(testing::Ref(stub_config)))
        .Times(1);

    ms::ApplicationSession app_session(
        mt::fake_shared(surface_factory),
        __LINE__,
        "Foo",
        std::make_shared<mtd::NullSnapshotStrategy>(),
        std::make_shared<msh::NullSessionListener>(),
        mt::fake_shared(sender));

    app_session.send_display_config(stub_config);
}

TEST(ApplicationSession, lifecycle_event_sender)
{
    using namespace ::testing;

    MirLifecycleState exp_state = mir_lifecycle_state_will_suspend;
    mtd::MockSurfaceFactory surface_factory;
    MockEventSink sender;

    ms::ApplicationSession app_session(
        mt::fake_shared(surface_factory),
        __LINE__,
        "Foo",
        std::make_shared<mtd::NullSnapshotStrategy>(),
        std::make_shared<msh::NullSessionListener>(),
        mt::fake_shared(sender));

    EXPECT_CALL(sender, handle_lifecycle_event(exp_state)).Times(1);

    app_session.set_lifecycle_state(mir_lifecycle_state_will_suspend);
}

TEST(ApplicationSession, process_id)
{
    using namespace ::testing;

    pid_t const pid{__LINE__};

    mtd::MockSurfaceFactory surface_factory;
    MockEventSink sender;

    ms::ApplicationSession app_session(
        mt::fake_shared(surface_factory),
        pid,
        "Foo",
        std::make_shared<mtd::NullSnapshotStrategy>(),
        std::make_shared<msh::NullSessionListener>(),
        mt::fake_shared(sender));

    EXPECT_THAT(app_session.process_id(), Eq(pid));
}
