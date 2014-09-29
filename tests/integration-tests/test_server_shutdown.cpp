/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/run_mir.h"

#include "mir_test_framework/display_server_test_fixture.h"

#include <gtest/gtest.h>

#include <thread>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mtf = mir_test_framework;

namespace
{

class ExceptionThrowingDisplayBufferCompositorFactory : public mc::DisplayBufferCompositorFactory
{
public:
    std::unique_ptr<mc::DisplayBufferCompositor>
        create_compositor_for(mg::DisplayBuffer&) override
    {
        struct ExceptionThrowingDisplayBufferCompositor : mc::DisplayBufferCompositor
        {
            void composite() override
            {
                throw std::runtime_error("ExceptionThrowingDisplayBufferCompositor");
            }
        };

        return std::unique_ptr<mc::DisplayBufferCompositor>(
            new ExceptionThrowingDisplayBufferCompositor{});
    }
};
}

TEST(ServerShutdownWithThreadException,
     server_releases_resources_on_abnormal_compositor_thread_termination)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mc::DisplayBufferCompositorFactory>
            the_display_buffer_compositor_factory() override
        {
            return display_buffer_compositor_factory(
                [this]()
                {
                    return std::make_shared<ExceptionThrowingDisplayBufferCompositorFactory>();
                });
        }
    };

    auto server_config = std::make_shared<ServerConfig>();

    std::thread server{
        [&]
        {
            EXPECT_THROW(
                mir::run_mir(*server_config, [](mir::DisplayServer&){}),
                std::runtime_error);
        }};

    server.join();

    std::weak_ptr<mir::graphics::Display> display = server_config->the_display();
    std::weak_ptr<mir::compositor::Compositor> compositor = server_config->the_compositor();
    std::weak_ptr<mir::frontend::Connector> connector = server_config->the_connector();
    std::weak_ptr<mir::input::InputManager> input_manager = server_config->the_input_manager();

    server_config.reset();

    EXPECT_EQ(0, display.use_count());
    EXPECT_EQ(0, compositor.use_count());
    EXPECT_EQ(0, connector.use_count());
    EXPECT_EQ(0, input_manager.use_count());
}
