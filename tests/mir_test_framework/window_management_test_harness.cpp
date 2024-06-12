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

#include "mir_test_framework/window_management_test_harness.h"
#include <mir/observer_multiplexer.h>
#include <mir/executor.h>
#include <mir/graphics/display_configuration_observer_multiplexer.h>
#include <mir/shell/abstract_shell.h>
#include <mir/shell/default_persistent_surface_store.h>
#include <mir/shell/surface_specification.h>
#include <mir/scene/surface_stack.h>
#include <mir/scene/session_coordinator.h>
#include <mir/scene/session_listener.h>
#include <mir/scene/session_container.h>
#include <mir/scene/application_session.h>
#include <mir/scene/surface_allocator.h>
#include <mir/scene/null_surface_observer.h>
#include <mir/scene/surface.h>
#include <mir/input/xkb_mapper.h>
#include <mir/input/input_sink.h>
#include <mir/wayland/weak.h>
#include <mir/report/null/shell_report.h>
#include "mir/test/doubles/stub_input_targeter.h"
#include "mir/test/doubles/stub_session.h"
#include "mir/test/doubles/null_prompt_session_manager.h"
#include "mir/test/doubles/stub_scene_report.h"
#include "mir/test/doubles/stub_decoration_manager.h"
#include "mir/test/doubles/stub_display_layout.h"
#include "mir/test/doubles/stub_input_seat.h"
#include "mir/test/doubles/stub_buffer_stream.h"
#include <miral/window_manager_tools.h>
#include <miral/minimal_window_manager.h>
#include "src/miral/basic_window_manager.h"

#include <queue>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
using namespace testing;

namespace
{
class WorkQueue
{
public:
    void enqueue(std::function<void()> const& f)
    {
        work_queue.push(f);
    }

    bool next()
    {
        if (work_queue.empty())
            return false;

        auto const& f = work_queue.front();
        f();
        work_queue.pop();
        return !work_queue.empty();
    }

    void run()
    {
        while (next()) {}
    }

private:
    std::queue<std::function<void()>> work_queue;
};

class HarnessSessionCoordinator : public ms::SessionCoordinator
{
public:
    explicit HarnessSessionCoordinator(
        std::shared_ptr<msh::SurfaceStack> const& surface_stack,
        std::shared_ptr<mir::ObserverRegistrar<mg::DisplayConfigurationObserver>> const& display_config_registrar) :
        observers{std::make_shared<SessionObservers>()},
        container{std::make_shared<ms::SessionContainer>()},
        surface_factory{std::make_shared<ms::SurfaceAllocator>(
            nullptr,
            std::make_shared<mtd::StubSceneReport>(),
            display_config_registrar)},
        surface_stack{surface_stack}
    {}

    void set_focus_to(std::shared_ptr<ms::Session> const& focus) override
    {
        observers->focused(focus);
    }

    void unset_focus() override
    {
        observers->unfocused();
    }

    auto open_session(
        pid_t pid,
        mir::Fd socket_fd,
        std::string const& name,
        std::shared_ptr<mir::frontend::EventSink> const& sink) -> std::shared_ptr<ms::Session> override
    {
        auto session = std::make_shared<ms::ApplicationSession>(
            surface_stack,
            surface_factory,
            pid,
            socket_fd,
            name,
            observers,
            sink,
            nullptr
        );
        container->insert_session(session);
        return session;
    }

    void close_session(std::shared_ptr<ms::Session> const& session) override
    {
        observers->stopping(session);
        container->remove_session(session);
    }

    auto successor_of(std::shared_ptr<ms::Session> const& ptr) const -> std::shared_ptr<ms::Session> override
    {
        return container->successor_of(ptr);
    }

    auto predecessor_of(std::shared_ptr<ms::Session> const& ptr) const -> std::shared_ptr<ms::Session> override
    {
        return container->predecessor_of(ptr);
    }

    void add_listener(std::shared_ptr<ms::SessionListener> const& ptr) override
    {
        observers->register_interest(ptr);
    }

    void remove_listener(std::shared_ptr<ms::SessionListener> const& ptr) override
    {
        observers->unregister_interest(*ptr);
    }

private:
    struct SessionObservers : public mir::ObserverMultiplexer<ms::SessionListener>
    {
        SessionObservers() : ObserverMultiplexer(mir::immediate_executor)
        {
        }

        void starting(std::shared_ptr<ms::Session> const& session) override
        {
            for_each_observer(&SessionListener::starting, session);
        }
        void stopping(std::shared_ptr<ms::Session> const& session) override
        {
            for_each_observer(&SessionListener::stopping, session);
        }
        void focused(std::shared_ptr<ms::Session> const& session) override
        {
            for_each_observer(&SessionListener::focused, session);
        }
        void unfocused() override
        {
            for_each_observer(&SessionListener::unfocused);
        }
        void surface_created(ms::Session& session, std::shared_ptr<ms::Surface> const& surface) override
        {
            for_each_observer(&SessionListener::surface_created, std::ref(session), surface);
        }
        void destroying_surface(ms::Session& session, std::shared_ptr<ms::Surface> const& surface) override
        {
            for_each_observer(&SessionListener::destroying_surface, std::ref(session), surface);
        }
        void buffer_stream_created(ms::Session& session, std::shared_ptr<mf::BufferStream> const& stream) override
        {
            for_each_observer(&SessionListener::buffer_stream_created, std::ref(session), stream);
        }
        void buffer_stream_destroyed(ms::Session& session, std::shared_ptr<mf::BufferStream> const& stream) override
        {
            for_each_observer(&SessionListener::buffer_stream_destroyed, std::ref(session), stream);
        }
    };

    std::shared_ptr<SessionObservers> observers;
    std::shared_ptr<ms::SessionContainer> const container;
    std::shared_ptr<ms::SurfaceFactory> surface_factory;
    std::shared_ptr<msh::SurfaceStack> surface_stack;
};

class VerifiableWindowManagementPolicy : public miral::WindowManagementPolicy
{
public:
    VerifiableWindowManagementPolicy(
        miral::WindowManagerTools const& wrapped,
        miral::WindowManagementPolicyBuilder const& builder,
        std::shared_ptr<mir_test_framework::WindowManagementVerifier> const& verifier)
        : policy{builder(wrapped)},
          verifier{verifier}
    {
    }

    void advise_begin() override
    {
        policy->advise_begin();
    }

    void advise_end() override
    {
        policy->advise_end();
    }

    auto place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& requested_specification) -> miral::WindowSpecification override
    {
        return policy->place_new_window(app_info, requested_specification);
    }

    void handle_window_ready(miral::WindowInfo& window_info) override
    {
        policy->handle_window_ready(window_info);
    }

    void handle_modify_window(miral::WindowInfo& window_info, miral::WindowSpecification const& modifications) override
    {
        policy->handle_modify_window(window_info, modifications);
    }

    void handle_raise_window(miral::WindowInfo& window_info) override
    {
        policy->handle_raise_window(window_info);
    }

    void handle_request_move(miral::WindowInfo& window_info, MirInputEvent const* input_event) override
    {
        policy->handle_request_move(window_info, input_event);
    }

    void handle_request_resize(miral::WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge) override
    {
        policy->handle_request_resize(window_info, input_event, edge);
    }

    auto confirm_inherited_move(
        miral::WindowInfo const& window_info,
        mir::geometry::Displacement movement) -> mir::geometry::Rectangle override
    {
        return policy->confirm_inherited_move(window_info, movement);
    }

    auto confirm_placement_on_display(
        miral::WindowInfo const& window_info, MirWindowState new_state,
        mir::geometry::Rectangle const& new_placement) -> mir::geometry::Rectangle override
    {
        return policy->confirm_placement_on_display(window_info, new_state, new_placement);
    }

    bool handle_keyboard_event(MirKeyboardEvent const* event) override
    {
        return policy->handle_keyboard_event(event);
    }

    bool handle_touch_event(MirTouchEvent const* event) override
    {
        return policy->handle_touch_event(event);
    }

    bool handle_pointer_event(MirPointerEvent const* event) override
    {
        return policy->handle_pointer_event(event);
    }

    void advise_new_app(miral::ApplicationInfo& application) override
    {
        policy->advise_new_app(application);
        verifier->advise_new_app(application);
    }

    void advise_delete_app(miral::ApplicationInfo const& application) override
    {
        policy->advise_delete_app(application);
        verifier->advise_delete_app(application);
    }

    void advise_new_window(miral::WindowInfo const& window_info) override
    {
        policy->advise_new_window(window_info);
        verifier->advise_new_window(window_info);
    }

    void advise_focus_lost(miral::WindowInfo const& window_info) override
    {
        policy->advise_focus_lost(window_info);
        verifier->advise_focus_lost(window_info);
    }

    void advise_focus_gained(miral::WindowInfo const& window_info) override
    {
        policy->advise_focus_gained(window_info);
        verifier->advise_focus_gained(window_info);
    }

    void advise_state_change(miral::WindowInfo const& window_info, MirWindowState state) override
    {
        policy->advise_state_change(window_info, state);
        verifier->advise_state_change(window_info, state);
    }

    void advise_move_to(miral::WindowInfo const& window_info, mir::geometry::Point top_left) override
    {
        policy->advise_move_to(window_info, top_left);
        verifier->advise_move_to(window_info, top_left);
    }

    void advise_resize(miral::WindowInfo const& window_info, mir::geometry::Size const& new_size) override
    {
        policy->advise_resize(window_info, new_size);
        verifier->advise_resize(window_info, new_size);
    }

    void advise_delete_window(miral::WindowInfo const& window_info) override
    {
        policy->advise_delete_window(window_info);
        verifier->advise_delete_window(window_info);
    }

    void advise_raise(std::vector<miral::Window> const& windows) override
    {
        policy->advise_raise(windows);
        verifier->advise_raise(windows);
    }

    void advise_adding_to_workspace(
        std::shared_ptr<miral::Workspace> const& workspace,
        std::vector<miral::Window> const& windows) override
    {
        policy->advise_adding_to_workspace(workspace, windows);
        verifier->advise_adding_to_workspace(workspace, windows);
    }

    void advise_removing_from_workspace(
        std::shared_ptr<miral::Workspace> const& workspace,
        std::vector<miral::Window> const& windows) override
    {
        policy->advise_removing_from_workspace(workspace, windows);
        verifier->advise_removing_from_workspace(workspace, windows);
    }

    void advise_output_create(miral::Output const& output) override
    {
        policy->advise_output_create(output);
        verifier->advise_output_create(output);
    }

    void advise_output_update(miral::Output const& updated, miral::Output const& original) override
    {
        policy->advise_output_update(updated, original);
        verifier->advise_output_update(updated, original);
    }

    void advise_output_delete(miral::Output const& output) override
    {
        policy->advise_output_delete(output);
        verifier->advise_output_delete(output);
    }

    void advise_application_zone_create(miral::Zone const& application_zone) override
    {
        policy->advise_application_zone_create(application_zone);
        verifier->advise_application_zone_create(application_zone);
    }

    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original) override
    {
        policy->advise_application_zone_update(updated, original);
        verifier->advise_application_zone_update(updated, original);
    }

    void advise_application_zone_delete(miral::Zone const& application_zone) override
    {
        policy->advise_application_zone_delete(application_zone);
        verifier->advise_application_zone_delete(application_zone);
    }

private:
    std::unique_ptr<miral::WindowManagementPolicy> const policy;
    std::shared_ptr<mir_test_framework::WindowManagementVerifier> const verifier;
};

}

struct mir_test_framework::WindowManagementTestHarness::Self : public ms::SurfaceObserver
{
    explicit Self(WindowManagementTestHarness* harness)
        : fake_display_configuration_observer_registrar{std::make_shared<mtd::FakeDisplayConfigurationObserverRegistrar>(
            harness->get_output_rectangles())},
          surface_stack{std::make_shared<ms::SurfaceStack>(std::make_shared<mtd::StubSceneReport>())},
          shell{std::make_shared<msh::AbstractShell>(
              std::make_shared<mtd::StubInputTargeter>(),
              surface_stack,
              std::make_shared<HarnessSessionCoordinator>(surface_stack, fake_display_configuration_observer_registrar),
              std::make_shared<mtd::NullPromptSessionManager>(),
              std::make_shared<mir::report::null::ShellReport>(),
              [&](msh::FocusController* focus_controller)
              {
                  return std::make_shared<miral::BasicWindowManager>(
                      focus_controller,
                      std::make_shared<mtd::StubDisplayLayout>(),
                      std::make_shared<msh::DefaultPersistentSurfaceStore>(),
                      *fake_display_configuration_observer_registrar,
                      [&](miral::WindowManagerTools const& tools)
                      {
                          return std::make_unique<VerifiableWindowManagementPolicy>(
                              tools,
                              harness->get_builder(),
                              harness->get_verifier());
                      }
                  );
              },
              std::make_shared<mtd::StubInputSeat>(),
              std::make_shared<mtd::StubDecorationManager>()
          )}
    {
    }

    void attrib_changed(mir::scene::Surface const*, MirWindowAttrib, int) override {}
    void window_resized_to(mir::scene::Surface const*, mir::geometry::Size const&) override {}
    void content_resized_to(mir::scene::Surface const*, mir::geometry::Size const&) override {}
    void moved_to(mir::scene::Surface const*, mir::geometry::Point const&) override {}
    void hidden_set_to(mir::scene::Surface const*, bool) override {}
    void frame_posted(mir::scene::Surface const*, mir::geometry::Rectangle const&) override {}
    void alpha_set_to(mir::scene::Surface const*, float) override {}
    void orientation_set_to(mir::scene::Surface const*, MirOrientation) override {}
    void transformation_set_to(mir::scene::Surface const*, glm::mat4 const&) override {}
    void reception_mode_set_to(mir::scene::Surface const*, mir::input::InputReceptionMode) override {}

    void cursor_image_set_to(
        mir::scene::Surface const*,
        std::weak_ptr<mir::graphics::CursorImage> const&) override {}

    void client_surface_close_requested(mir::scene::Surface const* surf) override
    {
        auto it = std::find_if(known_surfaces.begin(), known_surfaces.end(), [surf](std::shared_ptr<ms::Surface> const& s)
        {
            return s.get() == surf;
        });

        if (it != known_surfaces.end())
        {
            work_queue.enqueue([shell=shell, surface=*it]()
            {
                shell->destroy_surface(surface->session().lock(), surface);
            });
        }
    }

    void renamed(mir::scene::Surface const*, std::string const&) override {}

    void cursor_image_removed(mir::scene::Surface const*) override {}
    void placed_relative(mir::scene::Surface const*, mir::geometry::Rectangle const&) override {}
    void input_consumed(mir::scene::Surface const*, std::shared_ptr<MirEvent const> const&) override {}
    void application_id_set_to(mir::scene::Surface const*, std::string const&) override {}
    void depth_layer_set_to(mir::scene::Surface const*, MirDepthLayer) override {}
    void entered_output(mir::scene::Surface const*, mir::graphics::DisplayConfigurationOutputId const&) override {}
    void left_output(mir::scene::Surface const*, mir::graphics::DisplayConfigurationOutputId const&) override {}

    std::shared_ptr<mtd::FakeDisplayConfigurationObserverRegistrar> fake_display_configuration_observer_registrar;
    std::shared_ptr<ms::SurfaceStack> surface_stack;
    std::shared_ptr<msh::AbstractShell> shell;
    std::vector<std::shared_ptr<mir::compositor::BufferStream>> streams;
    std::vector<std::shared_ptr<ms::Surface>> known_surfaces;
    WorkQueue work_queue;
};

void mir_test_framework::WindowManagementTestHarness::SetUp()
{
    // Note: Allocation happens in the SetUp function because we need to call "get_builder"
    // and "get_verifier" from the derivative class. Creating these in the constructor
    // would make us accidentally call the pure virtual method.
    self = std::make_shared<Self>(this);
}

miral::Application mir_test_framework::WindowManagementTestHarness::open_application(
    std::string const& name)
{
    return self->shell->open_session(
        __LINE__,
        mir::Fd{mir::Fd::invalid},
        name,
        std::shared_ptr<mir::frontend::EventSink>());;
}

miral::Window mir_test_framework::WindowManagementTestHarness::create_window(
    miral::Application const& session,
    mir::geometry::Width const& width,
    mir::geometry::Height const& height)
{
    auto stream = std::make_shared<mir::test::doubles::StubBufferStream>();
    self->streams.push_back(stream);

    msh::SurfaceSpecification spec;
    spec.width = width;
    spec.height = height;
    spec.depth_layer = mir_depth_layer_application;
    spec.streams = std::vector<msh::StreamSpecification>();
    spec.streams.value().push_back(msh::StreamSpecification{
        stream,
        mir::geometry::Displacement{},
    });
    auto surface = self->shell->create_surface(
        session,
        {},
        spec,
        self,
        &mir::immediate_executor);
    self->known_surfaces.push_back(surface);
    self->shell->surface_ready(surface);

    return {session, surface};
}

void mir_test_framework::WindowManagementTestHarness::publish_event(MirEvent const& event)
{
    self->work_queue.enqueue([&]()
    {
        self->shell->handle(event);
    });
    self->work_queue.run();
}

void mir_test_framework::WindowManagementTestHarness::request_resize(
    miral::Window const& window,
    MirInputEvent const* event,
    MirResizeEdge edge)
{
    self->work_queue.enqueue([&]()
    {
        auto surface = window.operator std::shared_ptr<ms::Surface>();
        self->shell->request_resize(surface->session().lock(), surface, event, edge);
    });
    self->work_queue.run();
}

std::shared_ptr<mir::shell::AbstractShell> const& mir_test_framework::WindowManagementTestHarness::get_shell() const
{
    return self->shell;
}

std::shared_ptr<mir::scene::SurfaceStack> const& mir_test_framework::WindowManagementTestHarness::get_surface_stack() const
{
    return self->surface_stack;
}