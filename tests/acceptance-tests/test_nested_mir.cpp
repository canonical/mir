/*
 * Copyright © 2013 Canonical Ltd.
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
#include "mir/display_server.h"
#include "mir/run_mir.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_doubles/mock_gl.h"
#include "mir_test_doubles/mock_egl.h"

#ifndef ANDROID
#include "mir_test_doubles/mock_gbm.h"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mtf = mir_test_framework;

using namespace testing;

namespace
{
struct MockSessionMediatorReport : mf::NullSessionMediatorReport
{
    MockSessionMediatorReport()
    {
        EXPECT_CALL(*this, session_connect_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_disconnect_called(_)).Times(AnyNumber());

        // These are not needed for the 1st test, but they will be soon
        EXPECT_CALL(*this, session_create_surface_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_release_surface_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_next_buffer_called(_)).Times(AnyNumber());
    }

    MOCK_METHOD1(session_connect_called, void (std::string const&));
    MOCK_METHOD1(session_create_surface_called, void (std::string const&));
    MOCK_METHOD1(session_next_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_release_surface_called, void (std::string const&));
    MOCK_METHOD1(session_disconnect_called, void (std::string const&));
};

struct HostServerConfiguration : public mtf::TestingServerConfiguration
{
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
    static int const argc = 6;
    char const* argv[argc];

    FakeCommandLine(std::string const& host_socket)
    {
        char const** to = argv;
        for(auto from : { "--file", "NestedServer", "--host-socket", host_socket.c_str(), "--enable-input", "off"})
        {
            *to++ = from;
        }

        EXPECT_THAT(to - argv, Eq(argc)); // Check the array size matches parameter list
    }
};

struct NestedServerConfiguration : FakeCommandLine, public mir::DefaultServerConfiguration
{
    NestedServerConfiguration(std::string const& host_socket) :
        FakeCommandLine(host_socket),
        DefaultServerConfiguration(FakeCommandLine::argc, FakeCommandLine::argv)
    {
    }
};

struct NestedMockEGL : mir::test::doubles::MockEGL
{
    NestedMockEGL()
    {
        {
            InSequence init_before_terminate;
            EXPECT_CALL(*this, eglGetDisplay(_)).Times(1);

            EXPECT_CALL(*this, eglInitialize(_, _, _)).Times(1).WillRepeatedly(
                DoAll(WithArgs<1, 2>(Invoke(this, &NestedMockEGL::egl_initialize)), Return(EGL_TRUE)));

            EXPECT_CALL(*this, eglChooseConfig(_, _, _, _, _)).Times(AnyNumber()).WillRepeatedly(
                DoAll(WithArgs<2, 4>(Invoke(this, &NestedMockEGL::egl_choose_config)), Return(EGL_TRUE)));

            EXPECT_CALL(*this, eglTerminate(_)).Times(1);
        }

        EXPECT_CALL(*this, eglCreateWindowSurface(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(*this, eglMakeCurrent(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(*this, eglDestroySurface(_, _)).Times(AnyNumber());

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

struct NestedMockPlatform
#ifndef ANDROID
    : mir::test::doubles::MockGBM
#endif
{
    NestedMockPlatform()
    {
#ifndef ANDROID
        InSequence gbm_device_lifecycle;
        EXPECT_CALL(*this, gbm_create_device(_)).Times(1);
        EXPECT_CALL(*this, gbm_device_destroy(_)).Times(1);
#endif
    }
};

struct NestedMockGL : NiceMock<mir::test::doubles::MockGL>
{
    NestedMockGL() {}
};

template<class NestedServerConfiguration>
struct ClientConfig : mtf::TestingClientConfiguration
{
    ClientConfig(std::string const& host_socket) : host_socket(host_socket) {}

    std::string const host_socket;

    void exec() override
    {
        try
        {
            NestedMockPlatform mock_gbm;
            NestedMockEGL mock_egl;
            NestedMockGL mock_gl;
            NestedServerConfiguration nested_config(host_socket);

            mir::run_mir(nested_config, [](mir::DisplayServer& server){server.stop();});

            // TODO - remove FAIL() as we should exit (NB we need logic to cause exit).
            FAIL();
        }
        catch (std::exception const& x)
        {
            // TODO - this is only temporary until NestedPlatform is implemented.
            EXPECT_THAT(x.what(), HasSubstr("Platform::create_buffer_allocator is not implemented yet!"));
        }
    }
};
}

using TestNestedMir = mtf::BespokeDisplayServerTestFixture;

// TODO resolve problems running "nested" tests on android and running nested on GBM
#ifdef ANDROID
#define DISABLED_ON_ANDROID_AND_GBM(name) DISABLED_##name
#else
#define DISABLED_ON_ANDROID_AND_GBM(name) DISABLED_##name
#endif

TEST_F(TestNestedMir, DISABLED_ON_ANDROID_AND_GBM(nested_platform_connects_and_disconnects))
{
    struct MyHostServerConfiguration : HostServerConfiguration
    {
        void exec() override
        {
            InSequence seq;
            EXPECT_CALL(*mock_session_mediator_report, session_connect_called(_)).Times(1);
            EXPECT_CALL(*mock_session_mediator_report, session_disconnect_called(_)).Times(1);
        }
    };

    MyHostServerConfiguration host_config;
    ClientConfig<NestedServerConfiguration> client_config(host_config.the_socket_file());


    launch_server_process(host_config);
    launch_client_process(client_config);
}

//////////////////////////////////////////////////////////////////
// TODO the following tests were used in investigating lifetime issues.
// TODO they may not have much long term value, but decide that later

TEST(DisplayLeak, on_exit_display_objects_should_be_destroyed)
{
    struct MyServerConfiguration : mtf::TestingServerConfiguration
    {
        std::shared_ptr<mir::graphics::Display> the_display() override
        {
            auto const& temp = mtf::TestingServerConfiguration::the_display();
            my_display = temp;
            return temp;
        }

        std::weak_ptr<mir::graphics::Display> my_display;
    };

    MyServerConfiguration host_config;

    mir::run_mir(host_config, [](mir::DisplayServer& server){server.stop();});

    EXPECT_FALSE(host_config.my_display.lock()) << "after run_mir() exits the display should be released";
}

TEST_F(TestNestedMir, DISABLED_ON_ANDROID_AND_GBM(on_exit_display_objects_should_be_destroyed))
{
    struct MyNestedServerConfiguration : NestedServerConfiguration
    {
        // TODO clang says "error: inheriting constructors are not supported"
        // using NestedServerConfiguration::NestedServerConfiguration;
        MyNestedServerConfiguration(std::string const& host_socket) : NestedServerConfiguration(host_socket) {}

        std::shared_ptr<mir::graphics::Display> the_display() override
        {
            auto const& temp = NestedServerConfiguration::the_display();
            my_display = temp;
            return temp;
        }

        ~MyNestedServerConfiguration()
        {
            EXPECT_FALSE(my_display.lock()) << "after run_mir() exits the display should be released";
        }

        std::weak_ptr<mir::graphics::Display> my_display;
    };

    HostServerConfiguration host_config;
    ClientConfig<MyNestedServerConfiguration> client_config(host_config.the_socket_file());

    launch_server_process(host_config);
    launch_client_process(client_config);
}
