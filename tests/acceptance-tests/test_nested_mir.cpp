/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/frontend/session_mediator_report.h"
#include "mir/graphics/native_platform.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_configuration.h"
#include "mir/display_server.h"
#include "mir/run_mir.h"
#include "mir/shell/focus_controller.h"
#include "mir/scene/session.h"
#include "mir/shell/host_lifecycle_event_listener.h"

#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test/wait_condition.h"

#include "mir_test_doubles/mock_egl.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <future>
#include <mutex>
#include <condition_variable>

namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mtf = mir_test_framework;
namespace msh = mir::shell;
using namespace testing;

namespace
{
struct MockSessionMediatorReport : mf::SessionMediatorReport
{
    MockSessionMediatorReport()
    {
        EXPECT_CALL(*this, session_connect_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_disconnect_called(_)).Times(AnyNumber());

        // These are not needed for the 1st test, but they will be soon
        EXPECT_CALL(*this, session_create_surface_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_release_surface_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_next_buffer_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_exchange_buffer_called(_)).Times(AnyNumber());
    }

    MOCK_METHOD1(session_connect_called, void (std::string const&));
    MOCK_METHOD1(session_create_surface_called, void (std::string const&));
    MOCK_METHOD1(session_next_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_exchange_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_release_surface_called, void (std::string const&));
    MOCK_METHOD1(session_disconnect_called, void (std::string const&));
    MOCK_METHOD2(session_start_prompt_session_called, void (std::string const&, pid_t));
    MOCK_METHOD1(session_stop_prompt_session_called, void (std::string const&));

    void session_drm_auth_magic_called(const std::string&) override {};
    void session_configure_surface_called(std::string const&) override {};
    void session_configure_surface_cursor_called(std::string const&) override {};
    void session_configure_display_called(std::string const&) override {};
    void session_error(const std::string&, const char*, const std::string&) override {};
};

struct HostServerConfiguration : mtf::StubbedServerConfiguration
{
    using mtf::StubbedServerConfiguration::StubbedServerConfiguration;

    virtual std::shared_ptr<mf::SessionMediatorReport>  the_session_mediator_report()
    {
        if (!mock_session_mediator_report)
            mock_session_mediator_report = std::make_shared<MockSessionMediatorReport>();

        return mock_session_mediator_report;
    }

    std::shared_ptr<MockSessionMediatorReport> mock_session_mediator_report;
};

struct FakeCommandLine
{
    static int const argc = 7;
    char const* argv[argc];

    FakeCommandLine(std::string const& host_socket)
    {
        char const** to = argv;
        for(auto from : { "dummy-exe-name", "--file", "NestedServer", "--host-socket", host_socket.c_str(), "--enable-input", "off"})
        {
            *to++ = from;
        }

        EXPECT_THAT(to - argv, Eq(argc)); // Check the array size matches parameter list
    }
};

struct NativePlatformAdapter : mg::NativePlatform
{
    NativePlatformAdapter(std::shared_ptr<mg::Platform> const& adaptee) :
        adaptee(adaptee),
        ipc_ops(adaptee->make_ipc_operations())
    {
    }

    std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator() override
    {
        return adaptee->create_buffer_allocator();
    }

    std::shared_ptr<mg::PlatformIpcOperations> make_ipc_operations() const override
    {
        return ipc_ops;
    }

    std::shared_ptr<mg::InternalClient> create_internal_client() override
    {
        return adaptee->create_internal_client();
    }

    std::shared_ptr<mg::BufferWriter> make_buffer_writer() override
    {
        return adaptee->make_buffer_writer();
    }
    
    std::shared_ptr<mg::Platform> const adaptee;
    std::shared_ptr<mg::PlatformIpcOperations> const ipc_ops;
};

struct MockHostLifecycleEventListener : msh::HostLifecycleEventListener
{
    MockHostLifecycleEventListener(
        std::shared_ptr<msh::HostLifecycleEventListener> const& wrapped) :
        wrapped(wrapped)
    {
    }

    MOCK_METHOD1(lifecycle_event_occurred, void (MirLifecycleState));

    std::shared_ptr<msh::HostLifecycleEventListener> const wrapped;
};

struct NestedServerConfiguration : FakeCommandLine, public mir::DefaultServerConfiguration
{
    NestedServerConfiguration(
        std::string const& host_socket,
        std::shared_ptr<mg::Platform> const& graphics_platform) :
        FakeCommandLine(host_socket),
        DefaultServerConfiguration(FakeCommandLine::argc, FakeCommandLine::argv),
        graphics_platform(graphics_platform)
    {
    }

    std::shared_ptr<mg::NativePlatform> the_graphics_native_platform() override
    {
        return graphics_native_platform(
            [this]() -> std::shared_ptr<mg::NativePlatform>
            {
                return std::make_shared<NativePlatformAdapter>(graphics_platform);
            });
    }

    std::shared_ptr<mg::Platform> const graphics_platform;
};

struct NestedMockEGL : mir::test::doubles::MockEGL
{
    NestedMockEGL()
    {
        {
            InSequence init_before_terminate;
            EXPECT_CALL(*this, eglGetDisplay(_)).Times(1);
            EXPECT_CALL(*this, eglTerminate(_)).Times(1);
        }

        EXPECT_CALL(*this, eglCreateWindowSurface(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(*this, eglMakeCurrent(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(*this, eglDestroySurface(_, _)).Times(AnyNumber());

        EXPECT_CALL(*this, eglQueryString(_, _)).Times(AnyNumber());

        provide_egl_extensions();

        EXPECT_CALL(*this, eglChooseConfig(_, _, _, _, _)).Times(AnyNumber()).WillRepeatedly(
            DoAll(WithArgs<2, 4>(Invoke(this, &NestedMockEGL::egl_choose_config)), Return(EGL_TRUE)));

        EXPECT_CALL(*this, eglGetCurrentContext()).Times(AnyNumber());
        EXPECT_CALL(*this, eglCreatePbufferSurface(_, _, _)).Times(AnyNumber());

        EXPECT_CALL(*this, eglGetProcAddress(StrEq("eglCreateImageKHR"))).Times(AnyNumber());
        EXPECT_CALL(*this, eglGetProcAddress(StrEq("eglDestroyImageKHR"))).Times(AnyNumber());
        EXPECT_CALL(*this, eglGetProcAddress(StrEq("glEGLImageTargetTexture2DOES"))).Times(AnyNumber());

        {
            InSequence context_lifecycle;
            EXPECT_CALL(*this, eglCreateContext(_, _, _, _)).Times(AnyNumber()).WillRepeatedly(Return((EGLContext)this));
            EXPECT_CALL(*this, eglDestroyContext(_, _)).Times(AnyNumber()).WillRepeatedly(Return(EGL_TRUE));
        }
    }

private:
    void egl_initialize(EGLint* major, EGLint* minor) { *major = 1; *minor = 4; }
    void egl_choose_config(EGLConfig* config, EGLint*  num_config)
    {
        *config = this;
        *num_config = 1;
    }
};

class NestedMirRunner
{
public:
    NestedMirRunner(mir::ServerConfiguration& nested_config) :
        nested_run_mir{
            std::async(std::launch::async, [&]
            {
                mir::run_mir(nested_config, [&](mir::DisplayServer& server)
                    {
                        std::lock_guard<decltype(nested_mutex)> lock{nested_mutex};
                        nested_server = &server;
                        nested_cv.notify_one();
                    });
            })}
    {
        std::unique_lock<decltype(nested_mutex)> lock{nested_mutex};
        nested_cv.wait(lock, [&] { return nested_server != nullptr; });
    }

    ~NestedMirRunner() noexcept
    {
        std::lock_guard<decltype(nested_mutex)> lock{nested_mutex};
        nested_server->stop();
    }

private:
    std::mutex nested_mutex;
    std::condition_variable nested_cv;
    mir::DisplayServer* nested_server{nullptr};

    std::future<void> nested_run_mir;
};

std::vector<geom::Rectangle> const display_geometry
{
    {{  0, 0}, { 640,  480}},
    {{480, 0}, {1920, 1080}}
};

struct NestedServer : mtf::InProcessServer, HostServerConfiguration
{
    NestedServer() : HostServerConfiguration(display_geometry) {}

    NestedMockEGL mock_egl;
    mtf::UsingStubClientPlatform using_stub_client_platform;

    virtual mir::DefaultServerConfiguration& server_config()
    {
        return *this;
    }

    void SetUp() override
    {
        mtf::InProcessServer::SetUp();
        connection_string = new_connection();
    }

    void trigger_lifecycle_event(MirLifecycleState const lifecycle_state)
    {
        auto const app = the_focus_controller()->focussed_application().lock();

        EXPECT_TRUE(app != nullptr) << "Nested server not connected";

        if (app)
        {
           app->set_lifecycle_state(lifecycle_state);
        }
    }

    std::string connection_string;
};
}

TEST_F(NestedServer, nested_platform_connects_and_disconnects)
{
    NestedServerConfiguration nested_config{connection_string, the_graphics_platform()};

    InSequence seq;
    EXPECT_CALL(*mock_session_mediator_report, session_connect_called(_)).Times(1);
    EXPECT_CALL(*mock_session_mediator_report, session_disconnect_called(_)).Times(1);

    NestedMirRunner nested_mir{nested_config};
}

TEST_F(NestedServer, sees_expected_outputs)
{
    NestedServerConfiguration nested_config{connection_string, the_graphics_platform()};
    NestedMirRunner nested_mir{nested_config};

    auto const display = nested_config.the_display();
    auto const display_config = display->configuration();

    std::vector<geom::Rectangle> outputs;

     display_config->for_each_output([&] (mg::UserDisplayConfigurationOutput& output)
        {
            outputs.push_back(
                geom::Rectangle{
                    output.top_left,
                    output.modes[output.current_mode_index].size});
        });

    EXPECT_THAT(outputs, ContainerEq(display_geometry));
}

//////////////////////////////////////////////////////////////////
// TODO the following test was used in investigating lifetime issues.
// TODO it may not have much long term value, but decide that later.
TEST_F(NestedServer, on_exit_display_objects_should_be_destroyed)
{
    struct MyServerConfiguration : NestedServerConfiguration
    {
        using NestedServerConfiguration::NestedServerConfiguration;

        std::shared_ptr<mir::graphics::Display> the_display() override
        {
            auto const& temp = NestedServerConfiguration::the_display();
            my_display = temp;
            return temp;
        }

        std::weak_ptr<mir::graphics::Display> my_display;
    };

    MyServerConfiguration config{connection_string, the_graphics_platform()};

    NestedMirRunner{config};

    EXPECT_FALSE(config.my_display.lock()) << "after run_mir() exits the display should be released";
}

TEST_F(NestedServer, receives_lifecycle_events_from_host)
{
    struct MyServerConfiguration : NestedServerConfiguration
    {
        using NestedServerConfiguration::NestedServerConfiguration;

        std::shared_ptr<msh::HostLifecycleEventListener> the_host_lifecycle_event_listener() override
        {
            return host_lifecycle_event_listener([this]()
               -> std::shared_ptr<msh::HostLifecycleEventListener>
               {
                   return the_mock_host_lifecycle_event_listener();
               });
        }

        std::shared_ptr<MockHostLifecycleEventListener> the_mock_host_lifecycle_event_listener()
        {
            return mock_host_lifecycle_event_listener([this]
                {
                    return std::make_shared<MockHostLifecycleEventListener>(
                        NestedServerConfiguration::the_host_lifecycle_event_listener());
                });
        }

        mir::CachedPtr<MockHostLifecycleEventListener> mock_host_lifecycle_event_listener;
    };

    MyServerConfiguration nested_config{connection_string, the_graphics_platform()};

    NestedMirRunner nested_mir{nested_config};

    mir::test::WaitCondition events_processed;

    InSequence seq;
    EXPECT_CALL(*(nested_config.the_mock_host_lifecycle_event_listener()),
        lifecycle_event_occurred(mir_lifecycle_state_resumed)).Times(1);
    EXPECT_CALL(*(nested_config.the_mock_host_lifecycle_event_listener()),
        lifecycle_event_occurred(mir_lifecycle_state_will_suspend))
        .WillOnce(WakeUp(&events_processed));
    EXPECT_CALL(*(nested_config.the_mock_host_lifecycle_event_listener()),
        lifecycle_event_occurred(mir_lifecycle_connection_lost)).Times(AtMost(1));

    trigger_lifecycle_event(mir_lifecycle_state_resumed);
    trigger_lifecycle_event(mir_lifecycle_state_will_suspend);

    events_processed.wait_for_at_most_seconds(5);
}
