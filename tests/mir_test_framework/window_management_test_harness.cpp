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
#include <mir/scene/surface_observer.h>
#include <mir/shell/surface_specification.h>
#include <mir/shell/shell.h>
#include <mir/wayland/weak.h>
#include <miral/window_manager_tools.h>
#include <miral/set_window_management_policy.h>
#include <mir/log.h>
#include <mir/main_loop.h>
#include <future>

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
}

class mir_test_framework::WindowManagementTestHarness::Self final : public ms::SurfaceObserver
{
public:
    explicit Self(mir::Server& server) : server(server), tools{nullptr}
    {
    }

    void attrib_changed(ms::Surface const*, MirWindowAttrib, int) override {}
    void window_resized_to(ms::Surface const*, geom::Size const&) override {}
    void content_resized_to(ms::Surface const*, geom::Size const&) override {}
    void moved_to(ms::Surface const*, geom::Point const&) override {}
    void hidden_set_to(ms::Surface const*, bool) override {}
    void frame_posted(ms::Surface const*, geom::Rectangle const&) override {}
    void alpha_set_to(ms::Surface const*, float) override {}
    void orientation_set_to(ms::Surface const*, MirOrientation) override {}
    void transformation_set_to(ms::Surface const*, glm::mat4 const&) override {}
    void reception_mode_set_to(ms::Surface const*, mir::input::InputReceptionMode) override {}

    void cursor_image_set_to(
        ms::Surface const*,
        std::weak_ptr<mg::CursorImage> const&) override {}

    void client_surface_close_requested(ms::Surface const* surf) override
    {
        std::shared_ptr<ms::Surface> to_destroy = nullptr;
        {
            std::lock_guard lock{mutex};
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
        }

        if (to_destroy == nullptr)
        {
            mir::log_error("client_surface_close_requested: Unable to find surface in known_surfaces");
            return;
        }
        
        pending_actions.push_back([
            shell=server.the_shell(),
            to_destroy=to_destroy]
        {
            shell->destroy_surface(to_destroy->session().lock(), to_destroy);
        });
    }

    void renamed(ms::Surface const*, std::string const&) override {}

    void cursor_image_removed(ms::Surface const*) override {}
    void placed_relative(ms::Surface const*, geom::Rectangle const&) override {}
    void input_consumed(ms::Surface const*, std::shared_ptr<MirEvent const> const&) override {}
    void application_id_set_to(ms::Surface const*, std::string const&) override {}
    void depth_layer_set_to(ms::Surface const*, MirDepthLayer) override {}
    void entered_output(ms::Surface const*, mg::DisplayConfigurationOutputId const&) override {}
    void left_output(ms::Surface const*, mg::DisplayConfigurationOutputId const&) override {}
    void rescale_output(ms::Surface const*, mg::DisplayConfigurationOutputId const&) override {}

    void process_pending()
    {
        for (auto const& action : pending_actions)
            action();
        pending_actions.clear();
    }

    mir::Server& server;

    /// Lazily loaded window manager tools.
    miral::WindowManagerTools tools;

    std::mutex mutable mutex;

    /// A cache of the data associated with each surface. This is used for
    /// both discoverability of the surface and to ensure that the reference
    /// count of certain objects (e.g. the stream) do not drop below 0 until
    /// we want them to.
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

    mtd::FakeDisplay* display = nullptr;
};

mir_test_framework::WindowManagementTestHarness::WindowManagementTestHarness()
    : self{std::make_shared<Self>(server)}
{
}

void mir_test_framework::WindowManagementTestHarness::SetUp()
{
    miral::SetWindowManagementPolicy const policy(
        [&](miral::WindowManagerTools const& tools)
        {
            self->tools = tools;
            return get_builder()(tools);
        });
    policy(server);

    auto fake_display = std::make_unique<mtd::FakeDisplay>(get_output_rectangles());
    {
        std::lock_guard lock{self->mutex};
        self->display = fake_display.get();
    }
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
        name,
        std::shared_ptr<mir::frontend::EventSink>());
}

auto mir_test_framework::WindowManagementTestHarness::create_window(
    miral::Application const& session,
    miral::WindowSpecification const& window_spec) const -> miral::Window
{
    msh::SurfaceSpecification spec = make_surface_spec(window_spec);
    auto const stream = std::make_shared<mir::test::doubles::StubBufferStream>();
    spec.streams = std::vector<msh::StreamSpecification>();
    spec.streams.value().push_back(msh::StreamSpecification{
        stream,
        geom::Displacement{},
    });

    self->display->configuration()->for_each_output([&](mg::DisplayConfigurationOutput const& output)
    {
        spec.output_id = output.id;
    });
    auto surface = server.the_shell()->create_surface(
        session,
        {},
        spec,
        self,
        &mir::immediate_executor);
    {
        std::lock_guard lock{self->mutex};
        self->surface_cache.emplace_back(stream, surface);
    }
    server.the_shell()->surface_ready(surface);

    return {session, surface};
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

void mir_test_framework::WindowManagementTestHarness::request_focus(
    miral::Window const& window) const
{
    self->tools.select_active_window(window);
    self->process_pending();
}

auto mir_test_framework::WindowManagementTestHarness::focused(miral::Window const& window) const -> bool
{
    self->process_pending();
    return window.operator std::shared_ptr<ms::Surface>() == tools().active_window().operator std::shared_ptr<ms::Surface>();
}

auto mir_test_framework::WindowManagementTestHarness::tools() const -> miral::WindowManagerTools&
{
    return self->tools;
}

auto mir_test_framework::WindowManagementTestHarness::is_above(
    miral::Window const& a, miral::Window const& b) const -> bool
{
    return server.the_shell()->is_above(a.operator std::shared_ptr<ms::Surface>(), b.operator std::shared_ptr<ms::Surface>());
}
