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
#include <mir/executor.h>
#include <mir/scene/surface.h>
#include <mir/shell/surface_specification.h>
#include <mir/wayland/weak.h>
#include <miral/window_manager_tools.h>
#include <miral/minimal_window_manager.h>
#include <miral/set_window_management_policy.h>
#include "mir/test/doubles/fake_display.h"
#include "mir/test/doubles/stub_buffer_stream.h"
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

    auto next() -> bool
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

}

struct mir_test_framework::WindowManagementTestHarness::Self : public ms::SurfaceObserver
{
    explicit Self(WindowManagementTestHarness* harness)
         : harness{harness}
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
            work_queue.enqueue([shell=harness->server.the_shell(), surface=*it]()
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

    WindowManagementTestHarness* harness;
    std::vector<std::shared_ptr<mir::compositor::BufferStream>> streams;
    std::vector<std::shared_ptr<ms::Surface>> known_surfaces;
    WorkQueue work_queue;
    mtd::FakeDisplay* display = nullptr;
};

void mir_test_framework::WindowManagementTestHarness::SetUp()
{
    miral::SetWindowManagementPolicy policy(get_builder());
    policy(server);

    mir_test_framework::HeadlessInProcessServer::SetUp();
    self = std::make_shared<Self>(this);

    auto fake_display = std::make_unique<mtd::FakeDisplay>(get_output_rectangles());
    self->display = fake_display.get();
    preset_display(std::move(fake_display));
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
    mir::geometry::Width const& width,
    mir::geometry::Height const& height) -> miral::Window
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
    self->work_queue.enqueue([&]()
    {
        server.the_shell()->handle(event);
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
        server.the_shell()->request_resize(surface->session().lock(), surface, event, edge);
    });
    self->work_queue.run();
}

auto mir_test_framework::WindowManagementTestHarness::focused_surface() -> std::shared_ptr<mir::scene::Surface>
{
    return server.the_shell()->focused_surface();
}

