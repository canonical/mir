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

#include "mir_test/pipe.h"
#include "mir_test_framework/testing_server_configuration.h"
#include "mir_test_doubles/mock_input_manager.h"
#include "mir_test_doubles/null_display.h"
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

class MockCompositor : public mc::Compositor
{
public:
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());
};

class MockCommunicator : public mf::Communicator
{
public:
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());
};

class MockDisplay : public mtd::NullDisplay
{
public:
    MockDisplay(std::shared_ptr<mg::Display> const& display,
                int pause_signal, int resume_signal, int fd)
        : display{display},
          pause_signal{pause_signal},
          resume_signal{resume_signal},
          fd{fd}
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
            [conf_change_handler](int fd)
            {
                char c;
                if (read(fd, &c, 1) == 1)
                    conf_change_handler();
            });
    }

    void register_pause_resume_handlers(
        mg::EventHandlerRegister& handlers,
        mg::DisplayPauseHandler const& pause_handler,
        mg::DisplayResumeHandler const& resume_handler) override
    {
        handlers.register_signal_handler(
            {pause_signal},
            [pause_handler](int) { pause_handler(); });
        handlers.register_signal_handler(
            {resume_signal},
            [resume_handler](int) { resume_handler(); });
    }

    MOCK_METHOD0(pause, void());
    MOCK_METHOD0(resume, void());

private:
    std::shared_ptr<mg::Display> const display;
    int const pause_signal;
    int const resume_signal;
    int const fd;
};

class MockDisplayConfigurationPolicy : public mg::DisplayConfigurationPolicy
{
public:
    ~MockDisplayConfigurationPolicy() noexcept {}
    MOCK_METHOD1(apply_to, void(mg::DisplayConfiguration&));
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
            mock_compositor = std::make_shared<MockCompositor>();

        return mock_compositor;
    }

    std::shared_ptr<mf::Communicator> the_communicator() override
    {
        if (!mock_communicator)
            mock_communicator = std::make_shared<MockCommunicator>();

        return mock_communicator;
    }

    std::shared_ptr<mg::DisplayConfigurationPolicy> the_display_configuration_policy() override
    {
        if (!mock_conf_policy)
            mock_conf_policy = std::make_shared<MockDisplayConfigurationPolicy>();

        return mock_conf_policy;
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

    std::shared_ptr<MockCommunicator> the_mock_communicator()
    {
        the_communicator();
        return mock_communicator;
    }

    std::shared_ptr<MockDisplayConfigurationPolicy> the_mock_display_configuration_policy()
    {
        the_display_configuration_policy();
        return mock_conf_policy;
    }

    void emit_pause_event()
    {
        kill(getpid(), pause_signal);
    }

    void emit_resume_event()
    {
        kill(getpid(), resume_signal);
    }

    void emit_configuration_change_event()
    {
        if (write(p.write_fd(), "a", 1)) {}
    }

private:
    std::shared_ptr<MockCompositor> mock_compositor;
    std::shared_ptr<MockDisplay> mock_display;
    std::shared_ptr<MockCommunicator> mock_communicator;
    std::shared_ptr<MockDisplayConfigurationPolicy> mock_conf_policy;

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

    {
        InSequence s;

        /* Start */
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);

        /* Pause */
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
        EXPECT_CALL(*mock_display, pause()).Times(1);

        /* Resume */
        EXPECT_CALL(*mock_display, resume()).Times(1);
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);

        /* Stop */
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
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

    TestMainLoopServerConfig server_config;

    auto mock_compositor = server_config.the_mock_compositor();
    auto mock_display = server_config.the_mock_display();
    auto mock_communicator = server_config.the_mock_communicator();

    {
        InSequence s;

        /* Start */
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);

        /* Pause */
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
        EXPECT_CALL(*mock_display, pause()).Times(1);

        /* Stop */
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
    }

    mir::run_mir(server_config,
                 [&server_config](mir::DisplayServer&)
                 {
                    server_config.emit_pause_event();
                    kill(getpid(), SIGTERM);
                 });
}

TEST(DisplayServerMainLoopEvents, display_server_attempts_to_continue_on_pause_failure)
{
    using namespace testing;

    TestMainLoopServerConfig server_config;

    auto mock_compositor = server_config.the_mock_compositor();
    auto mock_display = server_config.the_mock_display();
    auto mock_communicator = server_config.the_mock_communicator();

    {
        InSequence s;

        /* Start */
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);

        /* Pause failure */
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
        EXPECT_CALL(*mock_display, pause())
            .WillOnce(Throw(std::runtime_error("")));

        /* Attempt to continue */
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);

        /* Stop */
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
    }

    mir::run_mir(server_config,
                 [&server_config](mir::DisplayServer&)
                 {
                    server_config.emit_pause_event();
                    kill(getpid(), SIGTERM);
                 });
}

TEST(DisplayServerMainLoopEvents, display_server_handles_configuration_change)
{
    using namespace testing;

    TestMainLoopServerConfig server_config;
    std::atomic<bool> configuration_changed{false};

    auto mock_compositor = server_config.the_mock_compositor();
    auto mock_display = server_config.the_mock_display();
    auto mock_communicator = server_config.the_mock_communicator();
    auto mock_conf_policy = server_config.the_mock_display_configuration_policy();

    {
        InSequence s;

        /* Start */
        EXPECT_CALL(*mock_communicator, start()).Times(1);
        EXPECT_CALL(*mock_compositor, start()).Times(1);

        /* Configuration change event */
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_conf_policy, apply_to(_)).Times(1);
        EXPECT_CALL(*mock_display, configure(_)).Times(1);
        EXPECT_CALL(*mock_compositor, start())
            .Times(1)
            .WillOnce(Assign(&configuration_changed, true));

        /* Stop */
        EXPECT_CALL(*mock_compositor, stop()).Times(1);
        EXPECT_CALL(*mock_communicator, stop()).Times(1);
    }

    mir::run_mir(server_config,
                 [&server_config,&configuration_changed](mir::DisplayServer&)
                 {
                    std::thread t{
                        [&server_config,&configuration_changed]
                        {
                           server_config.emit_configuration_change_event();
                           while (!configuration_changed)
                               std::this_thread::sleep_for(std::chrono::microseconds{500});
                           kill(getpid(), SIGTERM);
                        }};
                    t.detach();
                 });
}
