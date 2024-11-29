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
#include "mir/events/event_private.h"
#include "mir/graphics/buffer.h"
#include "mir/scene/surface_factory.h"
#include "mir/scene/null_session_listener.h"
#include "mir/scene/surface_event_source.h"
#include "mir/scene/output_properties_cache.h"
#include "mir/client_visible_error.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_surface_stack.h"
#include "mir/test/doubles/mock_surface.h"
#include "mir/test/doubles/mock_session_listener.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/stub_surface_factory.h"
#include "mir/test/doubles/stub_buffer_stream.h"
#include "mir/test/doubles/null_event_sink.h"
#include "mir/test/doubles/mock_event_sink.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_observer_registrar.h"
#include "mir/test/make_surface_spec.h"
#include "mir/graphics/display_configuration_observer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mw = mir::wayland;
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

MATCHER_P(EqPromptSessionEventState, state, "") {
  return arg.type() == mir_event_type_prompt_session_state_change && arg.to_prompt_session()->new_state() == state;
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
    void add_surface_below_top(std::shared_ptr<ms::Surface> const&, mi::InputReceptionMode) override
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
};

struct ApplicationSession : public testing::Test
{
    ApplicationSession()
        : event_sink(std::make_shared<mtd::NullEventSink>()),
          surface_observer(std::make_shared<ms::NullSurfaceObserver>()),
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
           event_sink,
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
           event_sink,
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
           event_sink,
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
           event_sink,
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
           event_sink,
           allocator);
    }


    std::shared_ptr<mtd::NullEventSink> const event_sink;
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
    MOCK_METHOD4(create_surface, std::shared_ptr<ms::Surface>(
        std::shared_ptr<ms::Session> const&,
        mw::Weak<mf::WlSurface> const&,
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

    EXPECT_CALL(mock_surface_factory, create_surface(_, _, _, _))
        .WillOnce(Return(mock_surface));
    EXPECT_CALL(surface_stack, add_surface(mock_surface,_));
    auto session = make_application_session(
        mt::fake_shared(surface_stack), mt::fake_shared(mock_surface_factory));

    auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surf = session->create_surface(nullptr, {}, params, surface_observer, nullptr);

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

    EXPECT_CALL(mock_surface_factory, create_surface(_, _, _, _))
        .WillOnce(Return(mock_surface));
    EXPECT_CALL(surface_stack, add_surface(mock_surface,_));
    auto session = make_application_session(
        mt::fake_shared(surface_stack), mt::fake_shared(mock_surface_factory));

    auto buffer_stream = session->create_buffer_stream(properties);
    auto const params = mt::make_surface_spec(session->create_buffer_stream(properties));
    auto surf = session->create_surface(nullptr, {}, params, surface_observer, nullptr);

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
    auto surf = session->create_surface(nullptr, {}, params, surface_observer, nullptr);

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
        session->create_surface(nullptr, {}, params, surface_observer, nullptr);
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
    auto surface1 = app_session->create_surface(nullptr, {}, params, nullptr, nullptr);
    auto surface2 = app_session->create_surface(nullptr, {}, params, nullptr, nullptr);
    auto surface3 = app_session->create_surface(nullptr, {}, params, nullptr, nullptr);

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
    auto surf1 = session1->create_surface(nullptr, {}, params, nullptr, nullptr);
    auto surf2 = session1->create_surface(nullptr, {}, params, nullptr, nullptr);

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
    auto surf = session->create_surface(nullptr, {}, params, nullptr, nullptr);

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
        surf[i] = app_session->create_surface(nullptr, {}, params, nullptr, nullptr);

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
    ON_CALL(surface_factory, create_surface(_, _, _, _)).WillByDefault(Return(mock_surface));
    NiceMock<mtd::MockSurfaceStack> surface_stack;
    auto app_session = make_application_session(mt::fake_shared(surface_stack), mt::fake_shared(surface_factory));

    {
        InSequence seq;
        EXPECT_CALL(*mock_surface, hide()).Times(1);
        EXPECT_CALL(*mock_surface, show()).Times(1);
    }

    auto const params = mt::make_surface_spec(app_session->create_buffer_stream(properties));
    auto surf = app_session->create_surface(nullptr, {}, params, surface_observer, nullptr);

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
        event_sink,
        allocator);

    EXPECT_THAT(app_session.process_id(), Eq(session_pid));
}

TEST_F(ApplicationSession, can_destroy_surface_bstream)
{
    auto session = make_application_session_with_stubs();
    auto const stream = session->create_buffer_stream(properties);
    auto const params = mt::make_surface_spec(stream);
    auto id = session->create_surface(nullptr, {}, params, surface_observer, nullptr);
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

    EXPECT_CALL(mock_surface_factory, create_surface(_, _, HasSingleStream(stream), _))
        .WillOnce(Return(make_mock_surface()));

    auto params = mt::make_surface_spec(stream);
    params.name = "Aardavks";
    params.type = mir_window_type_normal;

    session->create_surface(nullptr, {}, params, surface_observer, nullptr);
}

namespace
{
struct ApplicationSessionSender : public ApplicationSession
{
    ApplicationSessionSender() :
        app_session(
            stub_surface_stack,
            stub_surface_factory,
            pid,
            mir::Fd{mir::Fd::invalid},
            name,
            stub_session_listener,
            mt::fake_shared(sender),
            allocator)
    {
    }

    testing::NiceMock<mtd::MockEventSink> sender;
    ms::ApplicationSession app_session;
};
}

TEST_F(ApplicationSessionSender, error_sender)
{
    using namespace ::testing;

    class TestError : public mir::ClientVisibleError
    {
    public:
        TestError()
            : ClientVisibleError("Hello")
        {
        }

        MirErrorDomain domain() const noexcept override
        {
            return static_cast<MirErrorDomain>(42);
        }

        uint32_t code() const noexcept override
        {
            return 8u;
        }
    } error;
    EXPECT_CALL(sender, handle_error(Ref(error)))
        .Times(1);

    app_session.send_error(error);
}

TEST_F(ApplicationSessionSender, lifecycle_event_sender)
{
    using namespace ::testing;

    MirLifecycleState exp_state = mir_lifecycle_state_will_suspend;

    EXPECT_CALL(sender, handle_lifecycle_event(exp_state)).Times(1);
    app_session.set_lifecycle_state(mir_lifecycle_state_will_suspend);
}

TEST_F(ApplicationSessionSender, start_prompt_session)
{
    using namespace ::testing;

    EXPECT_CALL(sender, handle_event(EqPromptSessionEventState(mir_prompt_session_state_started))).Times(1);
    app_session.start_prompt_session();
}

TEST_F(ApplicationSessionSender, stop_prompt_session)
{
    using namespace ::testing;

    EXPECT_CALL(sender, handle_event(EqPromptSessionEventState(mir_prompt_session_state_stopped))).Times(1);
    app_session.stop_prompt_session();
}

namespace
{
class ObserverPreservingSurface : public mtd::MockSurface
{
public:
    void register_interest(std::weak_ptr<mir::scene::SurfaceObserver> const& observer) override
    {
        return BasicSurface::register_interest(observer);
    }

    void unregister_interest(mir::scene::SurfaceObserver const& observer) override
    {
        return BasicSurface::unregister_interest(observer);
    }
};

class ObserverPreservingSurfaceFactory : public ms::SurfaceFactory
{
public:
    std::shared_ptr<ms::Surface> create_surface(
        std::shared_ptr<ms::Session> const&,
        mw::Weak<mf::WlSurface> const&,
        std::list<ms::StreamInfo> const&,
        mir::shell::SurfaceSpecification const& params) override
    {
        using namespace testing;
        auto mock = std::make_shared<NiceMock<ObserverPreservingSurface>>();
        ON_CALL(*mock, size()).WillByDefault(Return(geom::Size{
            params.width.is_set() ? params.width.value() : geom::Width{1},
            params.height.is_set() ? params.height.value() : geom::Height{1}}));
        return mock;
    };
};

int calculate_dpi(geom::Size const& resolution, geom::Size const& size)
{
    float constexpr mm_per_inch = 25.4f;

    auto diagonal_mm = sqrt(size.height.as_int()*size.height.as_int()
                            + size.width.as_int()*size.width.as_int());
    auto diagonal_px = sqrt(resolution.height.as_int() * resolution.height.as_int()
                            + resolution.width.as_int() * resolution.width.as_int());

    return diagonal_px / diagonal_mm * mm_per_inch;
}

struct ApplicationSessionSurfaceOutput : public ApplicationSession
{
    ApplicationSessionSurfaceOutput() :
        high_dpi(static_cast<mg::DisplayConfigurationOutputId>(5), {3840, 2160}, {509, 286}, 2.5f, 60.0, mir_form_factor_monitor),
        projector(static_cast<mg::DisplayConfigurationOutputId>(2), {1280, 1024}, {800, 600}, 0.5f, 50.0, mir_form_factor_projector),
        stub_surface_factory{std::make_shared<ObserverPreservingSurfaceFactory>()},
        sender{std::make_shared<testing::NiceMock<mtd::MockEventSink>>()},
        observer{std::make_shared<ms::SurfaceEventSource>(mf::SurfaceId{1}, ms::OutputPropertiesCache{}, sender)},
        app_session(
            stub_surface_stack,
            stub_surface_factory,
            pid,
            mir::Fd{mir::Fd::invalid},
            name,
            stub_session_listener,
            sender,
            allocator)
    {
    }

    struct TestOutput
    {
        TestOutput(
            mg::DisplayConfigurationOutputId id,
            geom::Size const& resolution,
            geom::Size const& physical_size,
            float scale,
            double hz,
            MirFormFactor form_factor) :
            output{id, resolution, physical_size, mir_pixel_format_argb_8888, hz, true},
            form_factor{form_factor},
            scale{scale},
            dpi{calculate_dpi(resolution, physical_size)},
            width{resolution.width.as_int()},
            id{static_cast<uint32_t>(output.id.as_value())}
        {
            output.scale = scale;
            output.form_factor = form_factor;
        }

        mtd::StubDisplayConfigurationOutput output;
        MirFormFactor form_factor;
        float scale;
        int dpi;
        int width;
        uint32_t id;
    };

    TestOutput const high_dpi;
    TestOutput const projector;
    std::shared_ptr<ms::SurfaceFactory> const stub_surface_factory;
    std::shared_ptr<testing::NiceMock<mtd::MockEventSink>> sender;
    std::shared_ptr<ms::SurfaceEventSource> observer;
    ms::ApplicationSession app_session;
};
}

namespace
{
MATCHER_P(SurfaceOutputEventFor, output, "")
{
    using namespace testing;

    if (mir_event_get_type(arg) != mir_event_type_window_output)
    {
        *result_listener << "Event is not a MirWindowOutputEvent";
        return 0;
    }

    auto const event = mir_event_get_window_output_event(arg);

    if (output.output.current_mode_index >= output.output.modes.size())
        return false;

    auto const& mode = output.output.modes[output.output.current_mode_index];

    return
        ExplainMatchResult(
            Eq(output.dpi),
            mir_window_output_event_get_dpi(event),
            result_listener) &&
        ExplainMatchResult(
            Eq(output.form_factor),
            mir_window_output_event_get_form_factor(event),
            result_listener) &&
        ExplainMatchResult(
            Eq(output.scale),
            mir_window_output_event_get_scale(event),
            result_listener) &&
        ExplainMatchResult(
            Eq(mode.vrefresh_hz),
            mir_window_output_event_get_refresh_rate(event),
            result_listener) &&
        ExplainMatchResult(
            Eq(output.id),
            mir_window_output_event_get_output_id(event),
            result_listener);
}
}
