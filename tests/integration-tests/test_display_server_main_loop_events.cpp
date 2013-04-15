/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir/compositor/compositor.h"
#include "mir/graphics/display.h"
#include "mir/main_loop.h"

#include "mir_test_framework/testing_server_configuration.h"
#include "mir_test_doubles/mock_input_manager.h"
#include "mir/run_mir.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sys/types.h>
#include <unistd.h>

namespace mi = mir::input;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{

class MockCompositor : public mc::Compositor
{
public:
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());
};

class MockDisplay : public mg::Display
{
public:
    MockDisplay(std::shared_ptr<mg::Display> const& display,
                int pause_signal, int resume_signal)
        : display{display},
          pause_signal{pause_signal},
          resume_signal{resume_signal}
    {
    }

    mir::geometry::Rectangle view_area() const
    {
        return display->view_area();
    }

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
    {
        display->for_each_display_buffer(f);
    }

    std::shared_ptr<mg::DisplayConfiguration> configuration()
    {
        return display->configuration();
    }

    void register_pause_resume_handlers(
        mir::MainLoop& main_loop,
        std::function<void()> const& pause_handler,
        std::function<void()> const& resume_handler)
    {
        main_loop.register_signal_handler(
            {pause_signal},
            [pause_handler](int) { pause_handler(); });
        main_loop.register_signal_handler(
            {resume_signal},
            [resume_handler](int) { resume_handler(); });
    }

    MOCK_METHOD0(pause, void());
    MOCK_METHOD0(resume, void());

private:
    std::shared_ptr<mg::Display> const display;
    int const pause_signal;
    int const resume_signal;
};

class ServerConfig : public mtf::TestingServerConfiguration
{
public:
    std::shared_ptr<mi::InputManager> the_input_manager() override
    {
        if (!mock_input_manager)
        {
            mock_input_manager = std::make_shared<mtd::MockInputManager>();

            EXPECT_CALL(*mock_input_manager, start()).Times(1);
            EXPECT_CALL(*mock_input_manager, stop()).Times(1);
        }

        return mock_input_manager;
    }

    std::shared_ptr<mc::Compositor> the_compositor() override
    {
        if (!mock_compositor)
        {
            mock_compositor = std::make_shared<MockCompositor>();

            EXPECT_CALL(*mock_compositor, start()).Times(1);
            EXPECT_CALL(*mock_compositor, stop()).Times(1);
        }

        return mock_compositor;
    }

private:
    std::shared_ptr<mtd::MockInputManager> mock_input_manager;
    std::shared_ptr<MockCompositor> mock_compositor;
};


class PauseResumeServerConfig : public mtf::TestingServerConfiguration
{
public:
    PauseResumeServerConfig()
        : pause_signal{SIGUSR1}, resume_signal{SIGUSR2}
    {
    }

    std::shared_ptr<mg::Display> the_display() override
    {
        if (!mock_display)
        {
            auto display = mtf::TestingServerConfiguration::the_display();
            mock_display = std::make_shared<MockDisplay>(display,
                                                         pause_signal,
                                                         resume_signal);
        }

        return mock_display;
    }

    std::shared_ptr<mc::Compositor> the_compositor() override
    {
        if (!mock_compositor)
            mock_compositor = std::make_shared<MockCompositor>();

        return mock_compositor;
    }

    std::shared_ptr<MockDisplay> the_mock_display()
    {
        the_display();
        return mock_display;
    }

    std::shared_ptr<MockCompositor> the_mock_compositor()
    {
        the_compositor();
        return mock_compositor;
    }

    void emit_pause_event()
    {
        kill(getpid(), pause_signal);
    }

    void emit_resume_event()
    {
        kill(getpid(), resume_signal);
    }

private:
    std::shared_ptr<MockCompositor> mock_compositor;
    std::shared_ptr<MockDisplay> mock_display;

    int const pause_signal;
    int const resume_signal;
};

}

TEST(DisplayServerMainLoopEvents, display_server_shuts_down_properly_on_sigint)
{
    ServerConfig server_config;

    mir::run_mir(server_config,
                 [](mir::DisplayServer&)
                 {
                    kill(getpid(), SIGINT);
                 });
}

TEST(DisplayServerMainLoopEvents, display_server_shuts_down_properly_on_sigterm)
{
    ServerConfig server_config;

    mir::run_mir(server_config,
                 [](mir::DisplayServer&)
                 {
                    kill(getpid(), SIGTERM);
                 });
}

TEST(DisplayServerMainLoopEvents, display_server_components_pause_and_resume)
{
    using namespace testing;

    PauseResumeServerConfig server_config;

    auto mock_compositor = server_config.the_mock_compositor();
    auto mock_display = server_config.the_mock_display();

    {
        InSequence s;

        /* Start */
        EXPECT_CALL(*mock_compositor, start()).Times(1);

        /* Pause */
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_display, pause()).Times(1);

        /* Resume */
        EXPECT_CALL(*mock_display, resume()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);

        /* Stop */
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
    }

    mir::run_mir(server_config,
                 [&server_config](mir::DisplayServer&)
                 {
                    server_config.emit_pause_event();
                    server_config.emit_resume_event();
                    kill(getpid(), SIGTERM);
                 });
}

TEST(DisplayServerMainLoopEvents, display_server_quits_when_paused)
{
    using namespace testing;

    PauseResumeServerConfig server_config;

    auto mock_compositor = server_config.the_mock_compositor();
    auto mock_display = server_config.the_mock_display();

    {
        InSequence s;

        /* Start */
        EXPECT_CALL(*mock_compositor, start()).Times(1);

        /* Pause */
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_display, pause()).Times(1);

        /* Stop */
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
    }

    mir::run_mir(server_config,
                 [&server_config](mir::DisplayServer&)
                 {
                    server_config.emit_pause_event();
                    kill(getpid(), SIGTERM);
                 });
}
