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

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir/frontend/session_mediator_report.h"

#include "mir/run_mir.h"

#include "mir_test_doubles/mock_egl.h"

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
    static int const argc = 4;
    char const* argv[argc];

    FakeCommandLine(std::string const& host_socket)
    {
        char const** to = argv;
        for(auto from : { "--file", "NestedServer", "--nested-mode", host_socket.c_str()})
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

            EXPECT_CALL(*this, eglChooseConfig(_, _, _, _, _)).Times(1).WillRepeatedly(
                DoAll(WithArgs<2, 4>(Invoke(this, &NestedMockEGL::egl_choose_config)), Return(EGL_TRUE)));

            EXPECT_CALL(*this, eglTerminate(_)).Times(1);
        }

        {
            InSequence window_surface_lifecycle;
            EXPECT_CALL(*this, eglCreateWindowSurface(_, _, _, _)).Times(1).WillRepeatedly(Return((EGLSurface)this));
            EXPECT_CALL(*this, eglMakeCurrent(_, _, _, _)).Times(1).WillRepeatedly(Return(EGL_TRUE));
            EXPECT_CALL(*this, eglGetCurrentSurface(_)).Times(1).WillRepeatedly(Return((EGLSurface)this));
        }

        {
            InSequence context_lifecycle;
            EXPECT_CALL(*this, eglCreateContext(_, _, _, _)).Times(1).WillRepeatedly(Return((EGLContext)this));
            EXPECT_CALL(*this, eglDestroyContext(_, _)).Times(1).WillRepeatedly(Return(EGL_TRUE));
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

struct ClientConfig : mtf::TestingClientConfiguration
{
    ClientConfig(NestedServerConfiguration& nested_config) : nested_config(nested_config) {}

    NestedServerConfiguration& nested_config;

    void exec() override
    {
        NestedMockEGL mock_egl;

        try
        {
            // TODO remove this workaround for an existing issue:
            // avoids the graphics platform being created multiple times
            auto frig = nested_config.the_graphics_platform();

            std::cerr << "DEBUG pid=" << getpid() << " - " __FILE__ "(" << __LINE__ << ")\n";
            mir::run_mir(nested_config, [](mir::DisplayServer&){});
            // TODO - remove FAIL() as we should exit (NB we need logic to cause exit).
            std::cerr << "DEBUG pid=" << getpid() << " - " __FILE__ "(" << __LINE__ << ")\n";
            FAIL();
        }
        catch (std::exception const& x)
        {
            std::cerr << "DEBUG pid=" << getpid() << " - " __FILE__ "(" << __LINE__ << ")\n";
            // TODO - this is only temporary until NestedPlatform is implemented.
            EXPECT_THAT(x.what(), HasSubstr("NestedPlatform::create_buffer_allocator is not implemented yet!"));
        }
        std::cerr << "DEBUG pid=" << getpid() << " - " __FILE__ "(" << __LINE__ << ")\n";
    }
};
}

using TestNestedMir = mtf::BespokeDisplayServerTestFixture;

TEST_F(TestNestedMir, nested_platform_connects_and_disconnects)
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
    NestedServerConfiguration nested_config(host_config.the_socket_file());
    ClientConfig client_config(nested_config);


    launch_server_process(host_config);
    launch_client_process(client_config);
}

//////////////////////////////////////////////////////////////////

#include "mir/display_server.h"

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


TEST_F(TestNestedMir, on_exit_display_objects_should_be_destroyed)
{
    std::cerr << "DEBUG pid=" << getpid() << " - " __FILE__ "(" << __LINE__ << ")\n"
        << "DEBUG pid=" << getpid() << " ... starting test" << std::endl;

    struct MyNestedServerConfiguration : NestedServerConfiguration
    {
        using NestedServerConfiguration::NestedServerConfiguration;

        std::shared_ptr<mir::graphics::Display> the_display() override
        {
            std::cerr << "DEBUG pid=" << getpid() << " - " __FILE__ "(" << __LINE__ << ")\n";
            auto const& temp = NestedServerConfiguration::the_display();
            my_display = temp;
            std::cerr << "DEBUG pid=" << getpid() << " - " __FILE__ "(" << __LINE__ << ")\n"
            << "DEBUG pid=" << getpid() << "... my_display.lock() is " << my_display.lock().get() << std::endl;
            EXPECT_TRUE(!!my_display.lock()) << "pid=" << getpid() << " - the display should be acquired";
            return temp;
        }

        ~MyNestedServerConfiguration()
        {
            std::cerr << "DEBUG pid=" << getpid() << " - " __FILE__ "(" << __LINE__ << ")\n"
            << "DEBUG pid=" << getpid() << "... my_display.lock() is " << my_display.lock().get() << std::endl;
            EXPECT_FALSE(my_display.lock()) << "pid=" << getpid() << " - " << "after run_mir() exits the display should be released";
        }

        std::weak_ptr<mir::graphics::Display> my_display;
    };

    HostServerConfiguration host_config;
    MyNestedServerConfiguration nested_config(host_config.the_socket_file());
    ClientConfig client_config(nested_config);

    launch_server_process(host_config);
    launch_client_process(client_config);

    std::cerr << "DEBUG pid=" << getpid() << " - " __FILE__ "(" << __LINE__ << ")\n"
        << "DEBUG pid=" << getpid() << " ... end of test" << std::endl;
}
