/*
 * Copyright © 2014 Canonical Ltd.
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

#include "mir_toolkit/mir_client_library.h"

#include "src/server/frontend/session_mediator.h"

#include "mir/frontend/connection_context.h"
#include "mir/scene/session.h"
#include "mir/graphics/graphic_buffer_allocator.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"
#include "mir_test_doubles/fake_ipc_factory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <algorithm>
#include <condition_variable>
#include <mutex>

using namespace testing;

namespace
{
struct TrustedHelperSessionMediator : mir::frontend::SessionMediator
{
    using mir::frontend::SessionMediator::SessionMediator;

    std::function<void(std::shared_ptr<mir::frontend::Session> const&)> trusted_connect_handler() const override
    {
        return [this](std::shared_ptr<mir::frontend::Session> const& session)
            {
                auto const ss = std::dynamic_pointer_cast<mir::scene::Session>(session);
                client_pid = ss->process_id();
            };
    }

    pid_t mutable client_pid = 0;
};

std::shared_ptr<TrustedHelperSessionMediator> trusted_helper_mediator;

struct FakeIpcFactory : mir::test::doubles::FakeIpcFactory
{
    using mir::test::doubles::FakeIpcFactory::FakeIpcFactory;

    std::shared_ptr<mir::frontend::detail::DisplayServer> make_helper_mediator(
        std::shared_ptr<mir::frontend::Shell> const& shell,
        std::shared_ptr<mir::graphics::Platform> const& graphics_platform,
        std::shared_ptr<mir::frontend::DisplayChanger> const& changer,
        std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<mir::frontend::SessionMediatorReport> const& sm_report,
        std::shared_ptr<mir::frontend::EventSink> const& sink,
        std::shared_ptr<mir::frontend::Screencast> const& effective_screencast,
        mir::frontend::ConnectionContext const& connection_context)
    {
        trusted_helper_mediator = std::make_shared<TrustedHelperSessionMediator>(
            shell,
            graphics_platform,
            changer,
            buffer_allocator->supported_pixel_formats(),
            sm_report,
            sink,
            resource_cache(),
            effective_screencast,
            connection_context);

        return trusted_helper_mediator;
    }
};

struct MyServerConfiguration : mir_test_framework::StubbedServerConfiguration
{
    std::shared_ptr<FakeIpcFactory> mediator_factory;

    auto the_ipc_factory(
        std::shared_ptr<mir::frontend::Shell> const& shell,
        std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& allocator)
    -> std::shared_ptr<mir::frontend::ProtobufIpcFactory> override
    {
        return ipc_factory(
            [&]()
            {
                mediator_factory = std::make_shared<FakeIpcFactory>(
                    shell,
                    the_session_mediator_report(),
                    the_graphics_platform(),
                    the_frontend_display_changer(),
                    allocator,
                    the_screencast(),
                    the_session_authorizer());

                EXPECT_CALL(*mediator_factory,
                    make_mediator(_, _, _, _, _, _, _, _)).Times(AnyNumber())
                    .WillOnce(Invoke(mediator_factory.get(), &FakeIpcFactory::make_helper_mediator))
                    .WillRepeatedly(Invoke(mediator_factory.get(), &FakeIpcFactory::make_default_mediator));

                return mediator_factory;
            });
    }
};

using MyBasicClientServerFixture = mir_test_framework::BasicClientServerFixture<MyServerConfiguration>;

struct TrustSessionHelper : MyBasicClientServerFixture
{

    void TearDown()
    {
        trusted_helper_mediator.reset();
        MyBasicClientServerFixture::TearDown();
    }

    static std::size_t const arbritary_fd_request_count = 3;

    std::mutex mutex;
    std::size_t actual_fd_count = 0;
    int actual_fds[arbritary_fd_request_count] = {};
    std::condition_variable cv;
    bool called_back = false;

    bool wait_for_callback(std::chrono::milliseconds timeout)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        return cv.wait_for(lock, timeout, [this]{ return called_back; });
    }

    char client_connect_string[128] = {0};

    char const* fd_connect_string(int fd)
    {
        sprintf(client_connect_string, "fd://%d", fd);
        return client_connect_string;
    }
};

void client_fd_callback(MirConnection*, size_t count, int const* fds, void* context)
{
    auto const self = static_cast<TrustSessionHelper*>(context);

    std::unique_lock<decltype(self->mutex)> lock(self->mutex);
    self->actual_fd_count = count;

    std::copy(fds, fds+count, self->actual_fds);
    self->called_back = true;
    self->cv.notify_one();
}
}

TEST_F(TrustSessionHelper, can_get_fds_for_trusted_clients)
{
    mir_connection_new_fds_for_trusted_clients(connection, arbritary_fd_request_count, &client_fd_callback, this);
    EXPECT_TRUE(wait_for_callback(std::chrono::milliseconds(500)));

    EXPECT_THAT(actual_fd_count, Eq(arbritary_fd_request_count));
}

TEST_F(TrustSessionHelper, gets_pid_when_trusted_client_connects_over_fd)
{
    mir_connection_new_fds_for_trusted_clients(connection, 1, &client_fd_callback, this);
    wait_for_callback(std::chrono::milliseconds(500));

    auto const expected_pid = getpid();

    EXPECT_THAT(trusted_helper_mediator->client_pid, Eq(0)); // Before the client connects we don't have a pid
    auto client_connection = mir_connect_sync(fd_connect_string(actual_fds[0]), __PRETTY_FUNCTION__);
    EXPECT_THAT(trusted_helper_mediator->client_pid, Eq(expected_pid)); // After the client connects we do have a pid

    mir_connection_release(client_connection);
}
