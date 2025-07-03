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

#define MIR_LOG_COMPONENT "WindowManagementTestHarness"

#include "mir/test/doubles/fake_display.h"
#include "mir/test/doubles/stub_buffer_stream.h"
#include "mir_test_framework/window_management_test_harness.h"
#include <mir/executor.h>
#include <mir/scene/surface.h>
#include <mir/scene/null_surface_observer.h>
#include <mir/shell/surface_specification.h>
#include <mir/shell/shell.h>
#include <mir/wayland/weak.h>
#include <mir/log.h>
#include <mir/main_loop.h>
#include <mir/compositor/compositor.h>
#include <miral/window_manager_tools.h>
#include <miral/set_window_management_policy.h>
#include <miral/zone.h>
#include <mir/test/doubles/stub_display_configuration.h>
#include "mir/graphics/display_configuration_observer.h"
#include "mir/graphics/null_display_configuration_observer.h"
#include "miral/output.h"
#include "src/miral/window_manager_tools_implementation.h"

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
using namespace testing;

namespace
{
class CachedSurfaceData
{
public:
    CachedSurfaceData(
        std::shared_ptr<mc::BufferStream> const& stream,
        std::shared_ptr<ms::Surface> const& surface)
        : stream(stream),
          surface(surface)
    {
    }

    std::shared_ptr<mc::BufferStream> stream;
    std::shared_ptr<ms::Surface> surface;
};

/// Wraps a [miral::WindowManagerTools] and calls [on_change] whenever an action on the
/// tools would cause a change to the underlying system. This is important in this context
/// as it allows us to synchronously process work that has been triggered from other requests
/// which would have otherwise caused a deadlock.
class NotifyingWindowManagerToolsImplementation : public miral::WindowManagerToolsImplementation
{
public:
    NotifyingWindowManagerToolsImplementation(
        miral::WindowManagerTools const& tools,
        std::function<void()> const& on_change)
        : tools(tools), on_change(on_change) {}

    auto count_applications() const -> unsigned int override
    {
        return tools.count_applications();
    }

    void for_each_application(std::function<void(miral::ApplicationInfo& info)> const& func) override
    {
        tools.for_each_application(func);
    }

    auto find_application(std::function<bool(miral::ApplicationInfo const& info)> const& predicate) -> miral::Application override
    {
        return tools.find_application(predicate);
    }

    auto info_for(std::weak_ptr<mir::scene::Session> const& session) const -> miral::ApplicationInfo& override
    {
        return tools.info_for(session);
    }

    auto info_for(std::weak_ptr<mir::scene::Surface> const& surface) const -> miral::WindowInfo& override
    {
        return tools.info_for(surface);
    }

    auto info_for(miral::Window const& window) const -> miral::WindowInfo& override
    {
        return tools.info_for(window);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto info_for_window_id(std::string const& id) const -> miral::WindowInfo& override
    {
        return tools.info_for_window_id(id);
    }

    auto id_for_window(miral::Window const& window) const -> std::string override
    {
        return tools.id_for_window(window);
    }
#pragma GCC diagnostic pop

    void ask_client_to_close(miral::Window const& window) override
    {
        tools.ask_client_to_close(window);
        on_change();
    }

    auto active_window() const -> miral::Window override
    {
        return tools.active_window();
    }

    auto select_active_window(miral::Window const& hint) -> miral::Window override
    {
        auto window = tools.select_active_window(hint);
        on_change();
        return window;
    }

    void drag_active_window(mir::geometry::Displacement movement) override
    {
        tools.drag_active_window(movement);
        on_change();
    }

    void focus_next_application() override
    {
        tools.focus_next_application();
        on_change();
    }

    void focus_prev_application() override
    {
        tools.focus_prev_application();
        on_change();
    }

    void focus_next_within_application() override
    {
        tools.focus_next_within_application();
        on_change();
    }

    void focus_prev_within_application() override
    {
        tools.focus_prev_within_application();
        on_change();
    }

    auto window_to_select_application(const miral::Application app) const -> std::optional<miral::Window> override
    {
        auto window = tools.window_to_select_application(app);
        on_change();
        return window;
    }

    auto can_select_window(miral::Window const& window) const -> bool override
    {
        return tools.can_select_window(window);
    }

    auto window_at(mir::geometry::Point cursor) const -> miral::Window override
    {
        return tools.window_at(cursor);
    }

    auto active_output() -> mir::geometry::Rectangle const override
    {
        return tools.active_output();
    }

    void raise_tree(miral::Window const& root) override
    {
        tools.raise_tree(root);
        on_change();
    }

    void swap_tree_order(miral::Window const& first, miral::Window const& second) override
    {
        tools.swap_tree_order(first, second);
        on_change();
    }

    void send_tree_to_back(miral::Window const& root) override
    {
        tools.send_tree_to_back(root);
        on_change();
    }

    void modify_window(miral::WindowInfo& window_info, miral::WindowSpecification const& modifications) override
    {
        tools.modify_window(window_info, modifications);
        on_change();
    }

    void place_and_size_for_state(miral::WindowSpecification& modifications, miral::WindowInfo const& window_info) const override
    {
        tools.place_and_size_for_state(modifications, window_info);
        on_change();
    }

    auto create_workspace() -> std::shared_ptr<miral::Workspace> override
    {
        auto workspace = tools.create_workspace();
        on_change();
        return workspace;
    }

    void add_tree_to_workspace(miral::Window const& window, std::shared_ptr<miral::Workspace> const& workspace) override
    {
        tools.add_tree_to_workspace(window, workspace);
        on_change();
    }

    void remove_tree_from_workspace(miral::Window const& window, std::shared_ptr<miral::Workspace> const& workspace) override
    {
        tools.remove_tree_from_workspace(window, workspace);
        on_change();
    }

    void move_workspace_content_to_workspace(
        std::shared_ptr<miral::Workspace> const& to_workspace,
        std::shared_ptr<miral::Workspace> const& from_workspace) override
    {
        tools.move_workspace_content_to_workspace(to_workspace, from_workspace);
        on_change();
    }

    void for_each_workspace_containing(
        miral::Window const& window,
        std::function<void(std::shared_ptr<miral::Workspace> const&)> const& callback) override
    {
        tools.for_each_workspace_containing(window, callback);
    }

    void for_each_window_in_workspace(
        std::shared_ptr<miral::Workspace> const& workspace,
        std::function<void(miral::Window const& window)> const& callback) override
    {
        tools.for_each_window_in_workspace(workspace, callback);
    }

    void invoke_under_lock(std::function<void()> const& callback) override
    {
        tools.invoke_under_lock(callback);
    }

    void move_cursor_to(mir::geometry::PointF point) override
    {
        // move_cursor_to will queue work onto the linearising_executor.
        // Because that work is guaranteed to be linear, we can safely
        // queue another piece of work and wait for that to complete before
        // returning from this method.
        tools.move_cursor_to(point);
        std::mutex mtx;
        bool cursor_change_processed = false;
        std::condition_variable cv;
        mir::linearising_executor.spawn([&]
        {
            {
                std::lock_guard<std::mutex> lock(mtx);
                cursor_change_processed = true;
            }

            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]{ return cursor_change_processed; });
        on_change();
    }

    void drag_window(
        miral::Window const& window, mir::geometry::Displacement& movement) override
    {
        tools.drag_window(window, movement);
        on_change();
    }

    auto active_application_zone() -> miral::Zone override
    {
        return tools.active_application_zone();
    }
private:
    miral::WindowManagerTools tools;
    std::function<void()> const on_change;
};
}

class mir_test_framework::WindowManagementTestHarness::Self
{
public:
    explicit Self(mir::Server& server)
        : server(server),
          surface_observer(std::make_shared<SurfaceCloseRequestedObserver>([this](ms::Surface const* surf)
          {
              on_surface_closed(surf);
          }))
    {
    }

    void process_pending()
    {
        while (!pending_actions.empty())
        {
            decltype(pending_actions) actions;
            actions.swap(pending_actions);
            for (auto const& action : actions)
                action();
        }
    }

    void on_surface_closed(ms::Surface const* surf)
    {
        std::shared_ptr<ms::Surface> to_destroy = nullptr;
        auto const it = std::find_if(surface_cache.begin(), surface_cache.end(),
            [surf](CachedSurfaceData const& data)
            {
                return data.surface.get() == surf;
            });
        if (it != surface_cache.end())
        {
            to_destroy = it->surface;
            surface_cache.erase(it);
        }

        if (to_destroy == nullptr)
        {
            mir::log_error("client_surface_close_requested: Unable to find surface in known_surfaces");
            return;
        }

        pending_actions.emplace_back([
            shell=server.the_shell(),
            to_destroy=to_destroy]
        {
            shell->destroy_surface(to_destroy->session().lock(), to_destroy);
        });
    }

    class SurfaceCloseRequestedObserver : public ms::NullSurfaceObserver
    {
    public:
        explicit SurfaceCloseRequestedObserver(std::function<void(ms::Surface const* surf)> const& on_close_requested)
            : on_close_requested(on_close_requested) {}

        void client_surface_close_requested(ms::Surface const* surf) override
        {
            on_close_requested(surf);
        }

    private:
        std::function<void(ms::Surface const* surf)> on_close_requested;
    };

    mir::Server& server;
    std::unique_ptr<miral::WindowManagerTools> tools;
    std::unique_ptr<NotifyingWindowManagerToolsImplementation> impl;
    mtd::FakeDisplay* display = nullptr;
    std::shared_ptr<SurfaceCloseRequestedObserver> surface_observer;

    /// A cache of the data associated with each surface. This is used for
    /// both discoverability of the surface and to ensure that the reference
    /// count of certain objects (e.g. the stream) do not drop below 0 until
    /// we want them to.
    ///
    /// Access to this cache is not guarded by a lock as this cache can only
    /// ever be accessed by the testing thread during surface creation or
    /// deletion.
    std::vector<CachedSurfaceData> surface_cache;

    /// The BasicWindowManager expects some calls to be triggered asynchronously,
    /// specifically when they are in response to an input event. For example,
    /// dispatching an input event that would close a surface takes the internal
    /// lock of the BasicWindowManager. This action asks the BasicWindowManager
    /// to close the surface, which also wants to take the same lock.
    /// Hence, a deadlock when this code is run synchronously.
    ///
    /// The solution is to push this action to a processing queue that will be cleared
    /// after each action.
    std::vector<std::function<void()>> pending_actions;

};

mir_test_framework::WindowManagementTestHarness::WindowManagementTestHarness()
    : self{std::make_unique<Self>(server)}
{
}

mir_test_framework::WindowManagementTestHarness::~WindowManagementTestHarness() = default;

void mir_test_framework::WindowManagementTestHarness::SetUp()
{
    miral::SetWindowManagementPolicy const policy(
        [&](miral::WindowManagerTools const& tools)
        {
            self->impl = std::make_unique<NotifyingWindowManagerToolsImplementation>(tools, [self=self.get()]
            {
                self->process_pending();
            });
            self->tools = std::make_unique<miral::WindowManagerTools>(self->impl.get());

            // Note that we provide [tools] instead of [self->tools] here. This is because we
            // only want the [NotifyingWindowManagerToolsImplementation::on_change] callback to
            // be triggered at the toplevel caller function, instead of down the call stack.
            return get_builder()(tools);
        });
    policy(server);

    auto fake_display = std::make_unique<mtd::FakeDisplay>(get_initial_output_configs());
    self->display = fake_display.get();
    preset_display(std::move(fake_display));

    HeadlessInProcessServer::SetUp();
}

void mir_test_framework::WindowManagementTestHarness::TearDown()
{
    HeadlessInProcessServer::TearDown();
}

auto mir_test_framework::WindowManagementTestHarness::open_application(
    std::string const& name) const -> miral::Application
{
    return server.the_shell()->open_session(
        __LINE__,
        mir::Fd{mir::Fd::invalid},
        name);
}

auto mir_test_framework::WindowManagementTestHarness::create_window(
    miral::Application const& session,
    miral::WindowSpecification const& window_spec) const -> miral::Window
{
    msh::SurfaceSpecification spec = window_spec.to_surface_specification();
    auto const stream = std::make_shared<mir::test::doubles::StubBufferStream>();
    spec.streams = std::vector<msh::StreamSpecification>();
    spec.streams.value().push_back(msh::StreamSpecification{
        stream,
        geom::Displacement{},
    });

    auto surface = server.the_shell()->create_surface(
        session,
        {},
        spec,
        self->surface_observer,
        &mir::immediate_executor);
    self->surface_cache.emplace_back(stream, surface);
    server.the_shell()->surface_ready(surface);

    return tools().info_for(surface).window();
}

void mir_test_framework::WindowManagementTestHarness::publish_event(MirEvent const& event) const
{
    server.the_shell()->handle(event);
    self->process_pending();
}

void mir_test_framework::WindowManagementTestHarness::request_resize(
    miral::Window const& window,
    MirInputEvent const* event,
    MirResizeEdge edge) const
{
    auto const surface = window.operator std::shared_ptr<ms::Surface>();
    server.the_shell()->request_resize(surface->session().lock(), surface, event, edge);
    self->process_pending();
}

void mir_test_framework::WindowManagementTestHarness::request_move(
    miral::Window const& window, MirInputEvent const* event) const
{
    auto const surface = window.operator std::shared_ptr<ms::Surface>();
    server.the_shell()->request_move(surface->session().lock(), surface, event);
    self->process_pending();
}

void mir_test_framework::WindowManagementTestHarness::request_modify(
    miral::Window const& window,
    miral::WindowSpecification const& spec)
{
    server.the_shell()->modify_surface(
        window.application(),
        window.operator std::shared_ptr<ms::Surface>(),
        spec.to_surface_specification());
}

auto mir_test_framework::WindowManagementTestHarness::focused(miral::Window const& window) const -> bool
{
    self->process_pending();
    return window.operator std::shared_ptr<ms::Surface>() == tools().active_window().operator std::shared_ptr<ms::Surface>();
}

auto mir_test_framework::WindowManagementTestHarness::tools() const -> miral::WindowManagerTools&
{
    return *self->tools;
}

void mir_test_framework::WindowManagementTestHarness::for_each_output(std::function<void(miral::Output const&)> const& f) const
{
    self->display->configuration()->for_each_output([&](mir::graphics::DisplayConfigurationOutput const& output)
    {
        f(miral::Output(output));
    });
}

void mir_test_framework::WindowManagementTestHarness::update_outputs(
    std::vector<mir::graphics::DisplayConfigurationOutput> const& outputs) const
{
    class Observer : public mg::NullDisplayConfigurationObserver
    {
    public:
        void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const&) override
        {
            {
                std::lock_guard<std::mutex> lock(mtx);
                config_applied = true;
            }

            cv.notify_one();
        }

        void wait()
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&]{ return config_applied; });
        }

    private:
        std::condition_variable cv;
        std::mutex mtx;
        bool config_applied = false;
    };

    server.the_compositor()->stop();
    mtd::StubDisplayConfig const config(outputs);
    self->display->configure(config);
    self->display->emit_configuration_change_event(self->display->configuration());
    self->display->wait_for_configuration_change_handler();

    // We need to notify the display configuration observer so that
    // classes are made aware of the new rectangle. For example,
    // [SeatInputDeviceTracker] will update its bounds in response
    // to a reconfigure, which is required in order to move our pointer
    // into those outputs.
    //
    // The notifications are triggered asynchronously, so we add our
    // own listener to the end, thus guaranteeing that all notifications
    // have been triggered before we continue.
    auto const observer = std::make_shared<Observer>();
    server.the_display_configuration_observer_registrar()->register_interest(observer);
    server.the_display_configuration_observer()->configuration_applied(self->display->configuration());
    observer->wait();
    server.the_display_configuration_observer_registrar()->unregister_interest(*observer);
    server.the_compositor()->start();
}

auto mir_test_framework::WindowManagementTestHarness::is_above(
    miral::Window const& a, miral::Window const& b) const -> bool
{
    return server.the_shell()->is_above(a.operator std::shared_ptr<ms::Surface>(), b.operator std::shared_ptr<ms::Surface>());
}

auto mir_test_framework::WindowManagementTestHarness::output_configs_from_output_rectangles(
    std::vector<mir::geometry::Rectangle> const& output_rects) -> std::vector<mir::graphics::DisplayConfigurationOutput>
{
    std::vector<mir::graphics::DisplayConfigurationOutput> outputs;
    int id = 1;
    for (auto const& rect : output_rects)
    {
        mir::graphics::DisplayConfigurationOutput output
            {
                mir::graphics::DisplayConfigurationOutputId{id},
                mir::graphics::DisplayConfigurationCardId{0},
                mir::graphics::DisplayConfigurationLogicalGroupId{0},
                mir::graphics::DisplayConfigurationOutputType::vga,
                std::vector<MirPixelFormat>{mir_pixel_format_abgr_8888},
                {{rect.size, 60.0}},
                0, mir::geometry::Size{}, true, true, rect.top_left, 0,
                mir_pixel_format_abgr_8888, mir_power_mode_on,
                mir_orientation_normal,
                1.0f,
                mir_form_factor_monitor,
                mir_subpixel_arrangement_unknown,
                {},
                mir_output_gamma_unsupported,
                {},
                {}
            };

        outputs.push_back(output);
        ++id;
    }

    return outputs;
}
