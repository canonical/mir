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

#include <queue>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
using namespace testing;

struct mir_test_framework::WindowManagementTestHarness::Self : public ms::SurfaceObserver
{
    explicit Self() : tools{nullptr}
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
        std::lock_guard lock{mutex};
        auto it = std::find_if(known_surfaces.begin(), known_surfaces.end(), [surf](std::shared_ptr<ms::Surface> const& s)
        {
            return s.get() == surf;
        });

        if (it != known_surfaces.end())
        {
            surfaces_to_destroy.push_back(*it);
            known_surfaces.erase(it);
        }
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
    std::mutex mutable mutex;
    std::vector<std::shared_ptr<mc::BufferStream>> streams;
    std::vector<std::shared_ptr<ms::Surface>> known_surfaces;
    std::vector<std::shared_ptr<ms::Surface>> surfaces_to_destroy;
    mtd::FakeDisplay* display = nullptr;
    miral::WindowManagerTools tools;
};

mir_test_framework::WindowManagementTestHarness::WindowManagementTestHarness()
    : self{std::make_shared<Self>()}
{
}

void mir_test_framework::WindowManagementTestHarness::SetUp()
{
    miral::SetWindowManagementPolicy policy([&](miral::WindowManagerTools const& tools)
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

    mir_test_framework::HeadlessInProcessServer::SetUp();
}

void mir_test_framework::WindowManagementTestHarness::TearDown()
{
    mir_test_framework::HeadlessInProcessServer::TearDown();
}

auto mir_test_framework::WindowManagementTestHarness::open_application(
    std::string const& name) -> miral::Application
{
    return server.the_shell()->open_session(
        __LINE__,
        mir::Fd{mir::Fd::invalid},
        name,
        std::shared_ptr<mir::frontend::EventSink>());;
}

auto mir_test_framework::WindowManagementTestHarness::create_window(
    miral::Application const& session,
    mir::shell::SurfaceSpecification spec) -> miral::Window
{
    auto stream = std::make_shared<mir::test::doubles::StubBufferStream>();

    {
        std::lock_guard lock{self->mutex};
        self->streams.push_back(stream);
    }
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
    self->known_surfaces.push_back(surface);
    server.the_shell()->surface_ready(surface);

    return {session, surface};
}

void mir_test_framework::WindowManagementTestHarness::publish_event(MirEvent const& event)
{
    server.the_shell()->handle(event);
    std::lock_guard lock{self->mutex};
    for (auto const& surface : self->surfaces_to_destroy)
    {
        server.the_shell()->destroy_surface(surface->session().lock(), surface);
    }
}

void mir_test_framework::WindowManagementTestHarness::request_resize(
    miral::Window const& window,
    MirInputEvent const* event,
    MirResizeEdge edge)
{
    auto surface = window.operator std::shared_ptr<ms::Surface>();
    server.the_shell()->request_resize(surface->session().lock(), surface, event, edge);
}

void mir_test_framework::WindowManagementTestHarness::request_move(
    miral::Window const& window, MirInputEvent const* event)
{
    auto surface = window.operator std::shared_ptr<ms::Surface>();
    server.the_shell()->request_move(surface->session().lock(), surface, event);
}

void mir_test_framework::WindowManagementTestHarness::request_focus(miral::Window const& window)
{
    self->tools.select_active_window(window);
}

auto mir_test_framework::WindowManagementTestHarness::focused_surface() -> std::shared_ptr<ms::Surface>
{
    return server.the_shell()->focused_surface();
}

auto mir_test_framework::WindowManagementTestHarness::tools() -> miral::WindowManagerTools const&
{
    return self->tools;
}

auto mir_test_framework::WindowManagementTestHarness::is_above(
    miral::Window const& a, miral::Window const& b) const -> bool
{
    return server.the_shell()->is_above(a.operator std::shared_ptr<ms::Surface>(), b.operator std::shared_ptr<ms::Surface>());
}
