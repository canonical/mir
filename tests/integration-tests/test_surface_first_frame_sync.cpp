/*
 * Copyright © 2013-2015 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/compositor/compositor.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/display_listener.h"
#include "mir/renderer/renderer_factory.h"
#include "mir/renderer/renderer.h"
#include "mir/compositor/scene.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/display.h"
#include "mir/scene/legacy_scene_change_notification.h"
#include "mir/shell/shell.h"

#include "mir/test/doubles/stub_renderer.h"
#include "mir_test_framework/any_surface.h"
#include "mir_test_framework/basic_client_server_fixture.h"
#include "mir_test_framework/testing_server_configuration.h"
#include "mir/test/spin_wait.h"

#include "mir_toolkit/mir_client_library.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <unordered_map>
#include <atomic>

namespace mt = mir::test;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ms = mir::scene;

namespace
{

class SynchronousCompositor : public mc::Compositor
{
public:
    SynchronousCompositor(std::shared_ptr<mg::Display> const& display,
                          std::shared_ptr<mc::DisplayListener> const& display_listener,
                          std::shared_ptr<mc::Scene> const& scene,
                          std::shared_ptr<mc::DisplayBufferCompositorFactory> const& dbc_factory)
        : display{display},
          display_listener{display_listener},
          scene{scene}
    {
        display->for_each_display_sync_group([this, &dbc_factory](mg::DisplaySyncGroup& group)
        {
            group.for_each_display_buffer([this, &dbc_factory](mg::DisplayBuffer& display_buffer)
            {
                this->display_listener->add_display(display_buffer.view_area());
                auto dbc = dbc_factory->create_compositor_for(display_buffer);
                this->scene->register_compositor(dbc.get());
                display_buffer_compositor_map[&display_buffer] = std::move(dbc);
            });
        });
 
        auto notify = [this]()
        {
            this->display->for_each_display_sync_group([this](mg::DisplaySyncGroup& group)
            {
                group.for_each_display_buffer([this](mg::DisplayBuffer& display_buffer)
                {
                    auto& dbc = display_buffer_compositor_map[&display_buffer];
                    dbc->composite(this->scene->scene_elements_for(dbc.get(), display_buffer.view_area()));
                });
                group.post();
            });
        };
        auto notify2 = [notify](int) { notify(); };

        observer = std::make_shared<ms::LegacySceneChangeNotification>(notify, notify2);
    }

    ~SynchronousCompositor()
    {
        for(auto& dbc : display_buffer_compositor_map)
            scene->unregister_compositor(dbc.second.get());
    }

    void start()
    {
        scene->add_observer(observer);
    }

    void stop()
    {
        scene->remove_observer(observer);
    }

private:
    std::shared_ptr<mg::Display> const display;
    std::shared_ptr<mc::DisplayListener> const display_listener;
    std::shared_ptr<mc::Scene> const scene;
    std::unordered_map<mg::DisplayBuffer*,std::unique_ptr<mc::DisplayBufferCompositor>> display_buffer_compositor_map;
    
    std::shared_ptr<ms::Observer> observer;
};

class StubRenderer : public mtd::StubRenderer
{
public:
    StubRenderer(std::atomic<int>& render_operations)
        : render_operations{render_operations}
    {
    }

    void render(mg::RenderableList const& renderables) const override
    {
        for (auto const& r : renderables)
        {
            r->buffer(); // We need to consume a buffer to unblock client tests
            ++render_operations;
        }
    }

private:
    std::atomic<int>& render_operations;
};

struct ServerConfig : mtf::TestingServerConfiguration
{
    std::shared_ptr<mir::renderer::RendererFactory> the_renderer_factory() override
    {
        struct StubRendererFactory : mir::renderer::RendererFactory
        {
            StubRendererFactory(std::atomic<int>& render_operations)
                : render_operations{render_operations}
            {
            }

            std::unique_ptr<mir::renderer::Renderer> create_renderer_for(mg::DisplayBuffer&)
            {
                auto raw = new StubRenderer{render_operations};
                return std::unique_ptr<StubRenderer>(raw);
            }

            std::atomic<int>& render_operations;
        };

        if (!stub_renderer_factory)
            stub_renderer_factory = std::make_shared<StubRendererFactory>(render_operations);

        return stub_renderer_factory;
    }

    std::shared_ptr<mc::Compositor> the_compositor() override
    {
        if (!sync_compositor)
        {
            sync_compositor =
                std::make_shared<SynchronousCompositor>(
                    the_display(),
                    the_shell(),
                    the_scene(),
                    the_display_buffer_compositor_factory());
        }

        return sync_compositor;
    }

    std::atomic<int> render_operations{0};
    std::shared_ptr<mir::renderer::RendererFactory> stub_renderer_factory;
    std::shared_ptr<SynchronousCompositor> sync_compositor;
};

struct SurfaceFirstFrameSync : mtf::BasicClientServerFixture<ServerConfig>
{
    int number_of_executed_render_operations()
    {
        return server_configuration.render_operations;
    }
};

}

TEST_F(SurfaceFirstFrameSync, surface_not_rendered_until_buffer_is_pushed)
{
    auto window = mtf::make_any_surface(connection);

    ASSERT_TRUE(window != NULL);
    EXPECT_TRUE(mir_window_is_valid(window));
    EXPECT_STREQ(mir_window_get_error_message(window), "");

    /*
     * This test uses a synchronous compositor (instead of the default
     * multi-threaded one) to ensure we don't get a false negative
     * for this expectation. In the multi-threaded case we can get a
     * a false negative if the compositor doesn't get the chance to run at
     * all before we reach this point.
     */
    EXPECT_EQ(0, number_of_executed_render_operations());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_buffer_stream_swap_buffers_sync(
        mir_window_get_buffer_stream(window));
#pragma GCC diagnostic pop
    /* After submitting the buffer we should get some render operations */
    mir::test::spin_wait_for_condition_or_timeout(
        [this] { return number_of_executed_render_operations() > 0; },
        std::chrono::seconds{5});

    mir_window_release_sync(window);
}
