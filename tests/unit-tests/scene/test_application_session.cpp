/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/server/scene/application_session.h"
#include <mir/events/event.h>
#include <mir/events/prompt_session_event.h>
#include <mir/graphics/buffer.h>
#include <mir/scene/surface_factory.h>
#include <mir/scene/null_session_listener.h>
#include <mir/scene/null_surface_observer.h>
#include <mir/scene/output_properties_cache.h>
#include <mir/test/fake_shared.h>
#include <mir/test/doubles/mock_surface_stack.h>
#include <mir/test/doubles/mock_surface.h>
#include <mir/test/doubles/mock_session_listener.h>
#include <mir/test/doubles/stub_display_configuration.h>
#include <mir/test/doubles/stub_surface_factory.h>
#include <mir/test/doubles/stub_buffer_stream.h>
#include <mir/test/doubles/stub_buffer_allocator.h>
#include <mir/test/doubles/stub_observer_registrar.h>
#include <mir/test/make_surface_spec.h>
#include <mir/graphics/display_configuration_observer.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{
static std::shared_ptr<mtd::MockSurface> make_mock_surface()
{
    using namespace testing;
    auto surface = std::make_shared<NiceMock<mtd::MockSurface>>();
    ON_CALL(*surface, size()).WillByDefault(Return(geom::Size { 100, 100 }));
    return surface;
}

MATCHER_P(HasParent, parent, "")
{
    return arg.parent.lock() == parent;
}

struct StubSurfaceStack : public msh::SurfaceStack
{
    void raise(std::weak_ptr<ms::Surface> const&) override
    {
    }
    void raise(ms::SurfaceSet const&) override
    {
    }
    void add_surface(std::shared_ptr<ms::Surface> const&, mi::InputReceptionMode) override
    {
    }
    void remove_surface(std::weak_ptr<ms::Surface> const&) override
    {
    }
    auto surface_at(mir::geometry::Point) const -> std::shared_ptr<ms::Surface> override
    {
        return std::shared_ptr<ms::Surface>{};
    }
    void swap_z_order(ms::SurfaceSet const&, ms::SurfaceSet const&) override
    {
    }
    void send_to_back(ms::SurfaceSet const&) override
    {
    }

    auto is_above(std::weak_ptr<ms::Surface> const& /*a*/, std::weak_ptr<ms::Surface> const& /*b*/) const -> bool override
    {
        return false;
    }
};

struct ApplicationSession : public testing::Test
{
    ApplicationSession()
        : surface_observer(std::make_shared<ms::NullSurfaceObserver>()),
          stub_session_listener(std::make_shared<ms::NullSessionListener>()),
          stub_surface_stack(std::make_shared<StubSurfaceStack>()),
          pid(0),
          name("test-session-name")
    {
    }

    std::shared_ptr<ms::ApplicationSession> make_application_session_with_stubs()
    {
        return std::make_shared<ms::ApplicationSession>(
           stub_surface_stack,
           stub_surface_factory,
           pid,
           mir::Fd{mir::Fd::invalid},
           name,
           stub_session_listener,
           allocator);
    }

    std::shared_ptr<ms::ApplicationSession> make_application_session(
        std::shared_ptr<ms::SurfaceFactory> const& surface_factory)
    {
        return std::make_shared<ms::ApplicationSession>(
           stub_surface_stack,
           surface_factory,
           pid,
           mir::Fd{mir::Fd::invalid},
           name,
           stub_session_listener,
           allocator);
    }

    std::shared_ptr<ms::ApplicationSession> make_application_session(
        std::shared_ptr<msh::SurfaceStack> const& surface_stack,
        std::shared_ptr<ms::SurfaceFactory> const& surface_factory)
    {
        return std::make_shared<ms::ApplicationSession>(
           surface_stack,
           surface_factory,
           pid,
           mir::Fd{mir::Fd::invalid},
           name,
           stub_session_listener,
           allocator);
    }
    std::shared_ptr<ms::ApplicationSession> make_application_session_with_coordinator(
        std::shared_ptr<msh::SurfaceStack> const& surface_stack)
    {
        return std::make_shared<ms::ApplicationSession>(
           surface_stack,
           stub_surface_factory,
           pid,
           mir::Fd{mir::Fd::invalid},
           name,
           stub_session_listener,
           allocator);
    }

    std::shared_ptr<ms::ApplicationSession> make_application_session_with_listener(
        std::shared_ptr<ms::SessionListener> const& session_listener)
    {
        return std::make_shared<ms::ApplicationSession>(
           stub_surface_stack,
           stub_surface_factory,
           pid,
           mir::Fd{mir::Fd::invalid},
           name,
           session_listener,
           allocator);
    }


    std::shared_ptr<ms::NullSurfaceObserver> const surface_observer;
    std::shared_ptr<ms::NullSessionListener> const stub_session_listener;
    std::shared_ptr<StubSurfaceStack> const stub_surface_stack;
    std::shared_ptr<mtd::StubSurfaceFactory> const stub_surface_factory{std::make_shared<mtd::StubSurfaceFactory>()};
    std::shared_ptr<mtd::StubBufferStream> const stub_buffer_stream{std::make_shared<mtd::StubBufferStream>()};
    std::shared_ptr<mtd::StubBufferAllocator> const allocator{
        std::make_shared<mtd::StubBufferAllocator>()};
    pid_t pid;
    std::string name;
    mg::BufferProperties properties { geom::Size{1,1}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware };
    std::shared_ptr<mtd::StubObserverRegistrar<mir::graphics::DisplayConfigurationObserver>> const display_config_registrar =
        std::make_shared<mtd::StubObserverRegistrar<mir::graphics::DisplayConfigurationObserver>>();
};

struct MockSurfaceFactory : ms::SurfaceFactory
{
    MOCK_METHOD(
        std::shared_ptr<ms::Surface>,
        create_surface,
        (std::shared_ptr<ms::Session> const&,
         std::list<ms::StreamInfo> const&,
         msh::SurfaceSpecification const& params));
};
}

TEST_F(ApplicationSession, adds_created_surface_to_coordinator)
{
    using namespace ::testing;

    NiceMock<MockSurfaceFactory> mock_surface_factory;
    NiceMock<mtd::MockSurfaceStack> surface_stack;
    std::shared_ptr<ms::Surface> mock_surface = make_mock_surface();

    EXPECT_CALL(mock_surface_factory, create_surface(_, _, _))
        .WillOnce(Return(mock_surface));
    EXPECT_CALL(surface_stack, add_surface(mock_surface,_));
    auto session = make_application_session(
        mt::fake_shared(surface_stack), mt::fake_shared(mock_surface_factory));

    auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surf = session->create_surface(nullptr, params, surface_observer, nullptr);

    session->destroy_surface(surf);
}

TEST_F(ApplicationSession, attempt_to_destroy_non_existent_stream_throws)
{
    using namespace ::testing;
    NiceMock<MockSurfaceFactory> mock_surface_factory;
    NiceMock<mtd::MockSurfaceStack> surface_stack;
    auto session = make_application_session(
        mt::fake_shared(surface_stack), mt::fake_shared(mock_surface_factory));

    auto const made_up_stream = std::make_shared<mtd::MockBufferStream>();
    EXPECT_THROW({
        session->destroy_buffer_stream(made_up_stream);
    }, std::runtime_error);
}

TEST_F(ApplicationSession, can_destroy_buffer_stream_after_destroying_surface)
{
    using namespace ::testing;

    NiceMock<MockSurfaceFactory> mock_surface_factory;
    NiceMock<mtd::MockSurfaceStack> surface_stack;
    std::shared_ptr<ms::Surface> mock_surface = make_mock_surface();

    EXPECT_CALL(mock_surface_factory, create_surface(_, _, _))
        .WillOnce(Return(mock_surface));
    EXPECT_CALL(surface_stack, add_surface(mock_surface,_));
    auto session = make_application_session(
        mt::fake_shared(surface_stack), mt::fake_shared(mock_surface_factory));

    auto buffer_stream = session->create_buffer_stream(properties);
    auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surf = session->create_surface(nullptr, params, surface_observer, nullptr);

    session->destroy_surface(surf);
    session->destroy_buffer_stream(buffer_stream);
}

TEST_F(ApplicationSession, notifies_listener_of_create_and_destroy_surface)
{
    using namespace ::testing;

    mtd::MockSessionListener listener;
    EXPECT_CALL(listener, surface_created(_, _))
        .Times(1);
    EXPECT_CALL(listener, destroying_surface(_, _))
        .Times(1);

    auto session = make_application_session_with_listener(mt::fake_shared(listener));

    auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surf = session->create_surface(nullptr, params, surface_observer, nullptr);

    session->destroy_surface(surf);
}

TEST_F(ApplicationSession, notifies_listener_of_surface_destruction_via_session_destruction)
{
    using namespace ::testing;

    auto mock_surface = make_mock_surface();

    mtd::MockSessionListener listener;
    EXPECT_CALL(listener, surface_created(_, _)).Times(1);
    EXPECT_CALL(listener, destroying_surface(_, _)).Times(1);

    {
        auto session = make_application_session_with_listener(mt::fake_shared(listener));

        auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));
        session->create_surface(nullptr, params, surface_observer, nullptr);
    }
}

TEST_F(ApplicationSession, throws_on_destroy_invalid_surface)
{
    using namespace ::testing;

    auto app_session = make_application_session_with_stubs();

    auto const invalid_surface = std::make_shared<mtd::MockSurface>();

    EXPECT_THROW({
            app_session->destroy_surface(invalid_surface);
    }, std::runtime_error);
}

TEST_F(ApplicationSession, default_surface_is_first_surface)
{
    using namespace ::testing;

    auto app_session = make_application_session_with_stubs();

    auto const params = mt::make_surface_spec(app_session->create_buffer_stream(properties));
    auto surface1 = app_session->create_surface(nullptr, params, nullptr, nullptr);
    auto surface2 = app_session->create_surface(nullptr, params, nullptr, nullptr);
    auto surface3 = app_session->create_surface(nullptr, params, nullptr, nullptr);

    auto default_surf = app_session->default_surface();
    EXPECT_EQ(surface1, default_surf);
    app_session->destroy_surface(surface1);

    default_surf = app_session->default_surface();
    EXPECT_EQ(surface2, default_surf);
    app_session->destroy_surface(surface2);

    default_surf = app_session->default_surface();
    EXPECT_EQ(surface3, default_surf);
    app_session->destroy_surface(surface3);
}

TEST_F(ApplicationSession, foreign_surface_has_no_successor)
{
    auto session1 = make_application_session_with_stubs();
    auto const params = mt::make_surface_spec(session1->create_buffer_stream(properties));
    auto surf1 = session1->create_surface(nullptr, params, nullptr, nullptr);
    auto surf2 = session1->create_surface(nullptr, params, nullptr, nullptr);

    auto session2 = make_application_session_with_stubs();

    EXPECT_THROW({session2->surface_after(surf1);},
                 std::runtime_error);

    session1->destroy_surface(surf1);
    session1->destroy_surface(surf2);
}

TEST_F(ApplicationSession, surface_after_one_is_self)
{
    auto session = make_application_session_with_stubs();
    auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surf = session->create_surface(nullptr, params, nullptr, nullptr);

    EXPECT_EQ(surf, session->surface_after(surf));

    session->destroy_surface(surf);
}

TEST_F(ApplicationSession, surface_after_cycles_through_all)
{
    auto app_session = make_application_session_with_stubs();

    auto const params = mt::make_surface_spec(app_session->create_buffer_stream(properties));

    int const N = 3;
    std::shared_ptr<ms::Surface> surf[N];

    for (int i = 0; i < N; ++i)
    {
        surf[i] = app_session->create_surface(nullptr, params, nullptr, nullptr);

        if (i > 0)
        {
            ASSERT_NE(surf[i], surf[i-1]);
        }
    }

    for (int i = 0; i < N-1; ++i)
        ASSERT_EQ(surf[i+1], app_session->surface_after(surf[i]));

    EXPECT_EQ(surf[0], app_session->surface_after(surf[N-1]));

    for (int i = 0; i < N; ++i)
        app_session->destroy_surface(surf[i]);
}

TEST_F(ApplicationSession, session_visbility_propagates_to_surfaces)
{
    using namespace ::testing;

    auto mock_surface = make_mock_surface();

    NiceMock<MockSurfaceFactory> surface_factory;
    ON_CALL(surface_factory, create_surface(_, _, _)).WillByDefault(Return(mock_surface));
    NiceMock<mtd::MockSurfaceStack> surface_stack;
    auto app_session = make_application_session(mt::fake_shared(surface_stack), mt::fake_shared(surface_factory));

    {
        InSequence seq;
        EXPECT_CALL(*mock_surface, hide()).Times(1);
        EXPECT_CALL(*mock_surface, show()).Times(1);
    }

    auto const params = mt::make_surface_spec(app_session->create_buffer_stream(properties));
    auto surf = app_session->create_surface(nullptr, params, surface_observer, nullptr);

    app_session->hide();
    app_session->show();

    app_session->destroy_surface(surf);
}

TEST_F(ApplicationSession, process_id)
{
    using namespace ::testing;

    pid_t const session_pid{__LINE__};

    ms::ApplicationSession app_session(
        stub_surface_stack,
        stub_surface_factory,
        session_pid,
        mir::Fd{mir::Fd::invalid},
        name,
        std::make_shared<ms::NullSessionListener>(),
        allocator);

    EXPECT_THAT(app_session.process_id(), Eq(session_pid));
}

TEST_F(ApplicationSession, can_destroy_surface_bstream)
{
    auto session = make_application_session_with_stubs();
    auto const stream = session->create_buffer_stream(properties);
    auto const params = mt::make_surface_spec(stream);
    auto id = session->create_surface(nullptr, params, surface_observer, nullptr);
    session->destroy_buffer_stream(stream);
    EXPECT_FALSE(session->has_buffer_stream(stream));
    session->destroy_surface(id);
}

MATCHER(StreamEq, "")
{
    return (std::get<0>(arg).stream == std::get<1>(arg).stream) &&
        (std::get<0>(arg).displacement == std::get<1>(arg).displacement);
}

MATCHER_P(HasSingleStream, value, "")
{
    using namespace testing;
    EXPECT_THAT(arg.size(), Eq(1));
    if (arg.size() < 1 ) return false;
    EXPECT_THAT(arg.front().stream.get(), Eq(value.get()));
    return !(::testing::Test::HasFailure());
}

TEST_F(ApplicationSession, surface_uses_prexisting_buffer_stream_if_set)
{
    using namespace testing;

    NiceMock<MockSurfaceFactory> mock_surface_factory;

    geom::Size const buffer_size{geom::Width{1}, geom::Height{1}};

    mg::BufferProperties properties(buffer_size, mir_pixel_format_argb_8888, mg::BufferUsage::software);

    auto session = make_application_session(
        mt::fake_shared(mock_surface_factory));

    auto stream = session->create_buffer_stream(properties);

    EXPECT_CALL(mock_surface_factory, create_surface(_, HasSingleStream(stream), _))
        .WillOnce(Return(make_mock_surface()));

    auto params = mt::make_surface_spec(stream);
    params.name = "Aardavks";
    params.type = mir_window_type_normal;

    session->create_surface(nullptr, params, surface_observer, nullptr);
}

