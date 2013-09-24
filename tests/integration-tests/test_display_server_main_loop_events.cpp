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
#include "mir/frontend/communicator.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/main_loop.h"
#include "mir/display_changer.h"
#include "mir/pause_resume_listener.h"

#include "mir_test/pipe.h"
#include "mir_test_framework/testing_server_configuration.h"
#include "mir_test_doubles/mock_input_manager.h"
#include "mir_test_doubles/mock_compositor.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/mock_pause_resume_listener.h"
#include "mir/run_mir.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>
#include <thread>
#include <chrono>

#include <sys/types.h>
#include <unistd.h>

namespace mi = mir::input;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{

class MockCommunicator : public mf::Communicator
{
public:
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());
};

class MockDisplayChanger : public mir::DisplayChanger
{
public:
    MOCK_METHOD2(configure_for_hardware_change,
                 void(std::shared_ptr<mg::DisplayConfiguration> const& conf,
                      SystemStateHandling pause_resume_system));
};

class MockDisplay : public mtd::NullDisplay
{
public:
    MockDisplay(std::shared_ptr<mg::Display> const& display,
                int pause_signal, int resume_signal, int fd)
        : display{display},
          pause_signal{pause_signal},
          resume_signal{resume_signal},
          fd{fd},
          pause_handler_invoked_{false},
          resume_handler_invoked_{false},
          conf_change_handler_invoked_{false}
    {
    }

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f) override
    {
        display->for_each_display_buffer(f);
    }

    std::shared_ptr<mg::DisplayConfiguration> configuration() override
    {
        return display->configuration();
    }

    MOCK_METHOD1(configure, void(mg::DisplayConfiguration const&));

    void register_configuration_change_handler(
        mg::EventHandlerRegister& handlers,
        mg::DisplayConfigurationChangeHandler const& conf_change_handler) override
    {
        handlers.register_fd_handler(
            {fd},
            [this,conf_change_handler](int fd)
            {
                char c;
                if (read(fd, &c, 1) == 1)
                {
                    conf_change_handler();
                    conf_change_handler_invoked_ = true;
                }
            });
    }

    void register_pause_resume_handlers(
        mg::EventHandlerRegister& handlers,
        mg::DisplayPauseHandler const& pause_handler,
        mg::DisplayResumeHandler const& resume_handler) override
    {
        handlers.register_signal_handler(
            {pause_signal},
            [this,pause_handler](int)
            {
                pause_handler();
                pause_handler_invoked_ = true;
            });
        handlers.register_signal_handler(
            {resume_signal},
            [this,resume_handler](int)
            {
                resume_handler();
                resume_handler_invoked_ = true;
            });
    }

    MOCK_METHOD0(pause, void());
    MOCK_METHOD0(resume, void());

    bool pause_handler_invoked()
    {
        return pause_handler_invoked_.exchange(false);
    }

    bool resume_handler_invoked()
    {
        return resume_handler_invoked_.exchange(false);
    }

    bool conf_change_handler_invoked()
    {
        return conf_change_handler_invoked_.exchange(false);
    }

private:
    std::shared_ptr<mg::Display> const display;
    int const pause_signal;
    int const resume_signal;
    int const fd;
    std::atomic<bool> pause_handler_invoked_;
    std::atomic<bool> resume_handler_invoked_;
    std::atomic<bool> conf_change_handler_invoked_;
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
            mock_compositor = std::make_shared<mtd::MockCompositor>();

            EXPECT_CALL(*mock_compositor, start()).Times(1);
            EXPECT_CALL(*mock_compositor, stop()).Times(1);
        }

        return mock_compositor;
    }

private:
    std::shared_ptr<mtd::MockInputManager> mock_input_manager;
    std::shared_ptr<mtd::MockCompositor> mock_compositor;
};


class TestMainLoopServerConfig : public mtf::TestingServerConfiguration
{
public:
    TestMainLoopServerConfig()
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
                                                         resume_signal,
                                                         p.read_fd());
        }

        return mock_display;
    }

    std::shared_ptr<mc::Compositor> the_compositor() override
    {
        if (!mock_compositor)
            mock_compositor = std::make_shared<mtd::MockCompositor>();

        return mock_compositor;
    }

    std::shared_ptr<mf::Communicator> the_communicator() override
    {
        if (!mock_communicator)
            mock_communicator = std::make_shared<MockCommunicator>();

        return mock_communicator;
    }

    std::shared_ptr<mi::InputManager> the_input_manager() override
    {
        if (!mock_input_manager)
            mock_input_manager = std::make_shared<mtd::MockInputManager>();

        return mock_input_manager;
    }

    std::shared_ptr<mir::DisplayChanger> the_display_changer() override
    {
        if (!mock_display_changer)
            mock_display_changer = std::make_shared<MockDisplayChanger>();

        return mock_display_changer;
    }

    std::shared_ptr<MockDisplay> the_mock_display()
    {
        the_display();
        return mock_display;
    }

    std::shared_ptr<mtd::MockCompositor> the_mock_compositor()
    {
        the_compositor();
        return mock_compositor;
    }

    std::shared_ptr<MockCommunicator> the_mock_communicator()
    {
        the_communicator();
        return mock_communicator;
    }

    std::shared_ptr<mtd::MockInputManager> the_mock_input_manager()
    {
        the_input_manager();
        return mock_input_manager;
    }

    std::shared_ptr<MockDisplayChanger> the_mock_display_changer()
    {
        the_display_changer();
        return mock_display_changer;
    }

    void emit_pause_event_and_wait_for_handler()
    {
        kill(getpid(), pause_signal);
        while (!mock_display->pause_handler_invoked())
            std::this_thread::sleep_for(std::chrono::microseconds{500});
    }

    void emit_resume_event_and_wait_for_handler()
    {
        kill(getpid(), resume_signal);
        while (!mock_display->resume_handler_invoked())
            std::this_thread::sleep_for(std::chrono::microseconds{500});
    }

    void emit_configuration_change_event_and_wait_for_handler()
    {
        if (write(p.write_fd(), "a", 1)) {}
        while (!mock_display->conf_change_handler_invoked())
            std::this_thread::sleep_for(std::chrono::microseconds{500});
    }

private:
    std::shared_ptr<mtd::MockCompositor> mock_compositor;
    std::shared_ptr<MockDisplay> mock_display;
    std::shared_ptr<MockCommunicator> mock_communicator;
    std::shared_ptr<mtd::MockInputManager> mock_input_manager;
    std::shared_ptr<MockDisplayChanger> mock_display_changer;

    mt::Pipe p;
    int const pause_signal;
    int const resume_signal;
};

class TestPauseResumeListenerConfig : public mtf::TestingServerConfiguration
{
public:
    TestPauseResumeListenerConfig()
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
                                                         resume_signal,
                                                         p.read_fd());
        }

        return mock_display;
    }

    std::shared_ptr<mir::PauseResumeListener> the_pause_resume_listener() override
    {
        if (!mock_pause_resume_listener)
            mock_pause_resume_listener = std::make_shared<mtd::MockPauseResumeListener>();

        return mock_pause_resume_listener;
    }

    std::shared_ptr<MockDisplay> the_mock_display()
    {
        the_display();
        return mock_display;
    }

    std::shared_ptr<mtd::MockPauseResumeListener> the_mock_pause_resume_listener()
    {
        the_pause_resume_listener();
        return mock_pause_resume_listener;
    }

    void emit_pause_event_and_wait_for_handler()
    {
        kill(getpid(), pause_signal);
        while (!mock_display->pause_handler_invoked())
            std::this_thread::sleep_for(std::chrono::microseconds{500});
    }

    void emit_resume_event_and_wait_for_handler()
    {
        kill(getpid(), resume_signal);
        while (!mock_display->resume_handler_invoked())
            std::this_thread::sleep_for(std::chrono::microseconds{500});
    }

private:
    std::shared_ptr<MockDisplay> mock_display;
    std::shared_ptr<mtd::MockPauseResumeListener> mock_pause_resume_listener;

    mt::Pipe p;
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

    TestMainLoopServerConfig server_config;

    auto mock_compositor = server_config.the_mock_compositor();
    auto mock_display = server_config.the_mock_display();
    auto mock_communicator = server_config.the_mock_communicator();
    auto mock_input_manager = server_config.the_mock_input_manager();

    {
        InSequence s;

        /* Start */
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);
        EXPECT_CALL(*mock_input_manager, start()).Times(1);

        /* Pause */
        EXPECT_CALL(*mock_input_manager, stop()).Times(1);
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
        EXPECT_CALL(*mock_display, pause()).Times(1);

        /* Resume */
        EXPECT_CALL(*mock_display, resume()).Times(1);
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_input_manager, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);

        /* Stop */
        EXPECT_CALL(*mock_input_manager, stop()).Times(1);
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
    }

    mir::run_mir(server_config,
                 [&server_config](mir::DisplayServer&)
                 {
                    std::thread t{
                        [&]
                        {
                            server_config.emit_pause_event_and_wait_for_handler();
                            server_config.emit_resume_event_and_wait_for_handler();
                            kill(getpid(), SIGTERM);
                        }};
                    t.detach();
                 });
}

TEST(DisplayServerMainLoopEvents, display_server_quits_when_paused)
{
    using namespace testing;

    TestMainLoopServerConfig server_config;

    auto mock_compositor = server_config.the_mock_compositor();
    auto mock_display = server_config.the_mock_display();
    auto mock_communicator = server_config.the_mock_communicator();
    auto mock_input_manager = server_config.the_mock_input_manager();

    {
        InSequence s;

        /* Start */
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);
        EXPECT_CALL(*mock_input_manager, start()).Times(1);

        /* Pause */
        EXPECT_CALL(*mock_input_manager, stop()).Times(1);
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
        EXPECT_CALL(*mock_display, pause()).Times(1);

        /* Stop */
        EXPECT_CALL(*mock_input_manager, stop()).Times(1);
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
    }

    mir::run_mir(server_config,
                 [&server_config](mir::DisplayServer&)
                 {
                    std::thread t{
                        [&]
                        {
                            server_config.emit_pause_event_and_wait_for_handler();
                            kill(getpid(), SIGTERM);
                        }};
                    t.detach();
                 });
}

TEST(DisplayServerMainLoopEvents, display_server_attempts_to_continue_on_pause_failure)
{
    using namespace testing;

    TestMainLoopServerConfig server_config;

    auto mock_compositor = server_config.the_mock_compositor();
    auto mock_display = server_config.the_mock_display();
    auto mock_communicator = server_config.the_mock_communicator();
    auto mock_input_manager = server_config.the_mock_input_manager();

    {
        InSequence s;

        /* Start */
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);
        EXPECT_CALL(*mock_input_manager, start()).Times(1);

        /* Pause failure */
        EXPECT_CALL(*mock_input_manager, stop()).Times(1);
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
        EXPECT_CALL(*mock_display, pause())
            .WillOnce(Throw(std::runtime_error("")));

        /* Attempt to continue */
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);
        EXPECT_CALL(*mock_input_manager, start()).Times(1);

        /* Stop */
        EXPECT_CALL(*mock_input_manager, stop()).Times(1);
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
    }

    mir::run_mir(server_config,
                 [&server_config](mir::DisplayServer&)
                 {
                    std::thread t{
                        [&]
                        {
                            server_config.emit_pause_event_and_wait_for_handler();
                            kill(getpid(), SIGTERM);
                        }};
                    t.detach();
                 });
}

TEST(DisplayServerMainLoopEvents, display_server_handles_configuration_change)
{
    using namespace testing;

    TestMainLoopServerConfig server_config;

    auto mock_compositor = server_config.the_mock_compositor();
    auto mock_display = server_config.the_mock_display();
    auto mock_communicator = server_config.the_mock_communicator();
    auto mock_input_manager = server_config.the_mock_input_manager();
    auto mock_display_changer = server_config.the_mock_display_changer();

    {
        InSequence s;

        /* Start */
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);
        EXPECT_CALL(*mock_input_manager, start()).Times(1);

        /* Configuration change event */
        EXPECT_CALL(*mock_display_changer,
                    configure_for_hardware_change(_, mir::DisplayChanger::PauseResumeSystem))
            .Times(1);

        /* Stop */
        EXPECT_CALL(*mock_input_manager, stop()).Times(1);
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
    }

    mir::run_mir(server_config,
                 [&](mir::DisplayServer&)
                 {
                    std::thread t{
                        [&]
                        {
                            server_config.emit_configuration_change_event_and_wait_for_handler();
                            kill(getpid(), SIGTERM);
                        }};
                    t.detach();
                 });
}

TEST(DisplayServerMainLoopEvents, postpones_configuration_when_paused)
{
    using namespace testing;

    TestMainLoopServerConfig server_config;

    auto mock_compositor = server_config.the_mock_compositor();
    auto mock_display = server_config.the_mock_display();
    auto mock_communicator = server_config.the_mock_communicator();
    auto mock_input_manager = server_config.the_mock_input_manager();
    auto mock_display_changer = server_config.the_mock_display_changer();

    {
        InSequence s;

        /* Start */
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);
        EXPECT_CALL(*mock_input_manager, start()).Times(1);

        /* Pause event */
        EXPECT_CALL(*mock_input_manager, stop()).Times(1);
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
        EXPECT_CALL(*mock_display, pause()) .Times(1);

        /* Resume and reconfigure event */
        EXPECT_CALL(*mock_display, resume()).Times(1);
        EXPECT_CALL(*mock_communicator, start()).Times(1);

        EXPECT_CALL(*mock_display_changer,
                    configure_for_hardware_change(_, mir::DisplayChanger::RetainSystemState))
            .Times(1);

        EXPECT_CALL(*mock_input_manager, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);

        /* Stop */
        EXPECT_CALL(*mock_input_manager, stop()).Times(1);
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
    }

    mir::run_mir(server_config,
                 [&](mir::DisplayServer&)
                 {
                    std::thread t{
                        [&]
                        {
                            server_config.emit_pause_event_and_wait_for_handler();
                            server_config.emit_configuration_change_event_and_wait_for_handler();
                            server_config.emit_resume_event_and_wait_for_handler();

                            kill(getpid(), SIGTERM);
                        }};
                    t.detach();
                 });
}

TEST(DisplayServerMainLoopEvents, pause_resume_listener)
{
    using namespace testing;

    TestPauseResumeListenerConfig server_config;

    auto mock_display = server_config.the_mock_display();
    auto mock_pause_resume_listener = server_config.the_mock_pause_resume_listener();

    {
        InSequence s;

        EXPECT_CALL(*mock_display, pause()).Times(1);
        EXPECT_CALL(*mock_pause_resume_listener, paused()).Times(1);
        EXPECT_CALL(*mock_display, resume()).Times(1);
        EXPECT_CALL(*mock_pause_resume_listener, resumed()).Times(1);
    }

    mir::run_mir(server_config,
                 [&server_config](mir::DisplayServer&)
                 {
                    std::thread t{
                        [&]
                        {
                            server_config.emit_pause_event_and_wait_for_handler();
                            server_config.emit_resume_event_and_wait_for_handler();
                            kill(getpid(), SIGTERM);
                        }};
                    t.detach();
                 });
}
