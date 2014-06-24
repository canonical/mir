/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/client/rpc/stream_transport.h"
#include "src/client/rpc/asio_socket_transport.h"

#include "mir_test/signal.h"

#include <cstdint>
#include <thread>
#include <system_error>
#include <array>
#include <atomic>
#include <poll.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mclr = mir::client::rpc;

namespace
{
class MockObserver : public mclr::StreamTransport::Observer
{
public:
    MOCK_METHOD0(on_data_available, void());
    MOCK_METHOD0(on_disconnected, void());
};
}

class AsioSocketTransport : public ::testing::Test
{
public:
    AsioSocketTransport()
    {
        int socket_fds[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds) < 0)
        {
            throw std::system_error(errno, std::system_category());
        }

        test_fd = socket_fds[0];
        transport_fd = socket_fds[1];
    }

    virtual ~AsioSocketTransport()
    {
        // Transport should have closed its fd

        // We don't care about errors.
        close(test_fd);
    }

    int test_fd;
    int transport_fd;
};

TEST_F(AsioSocketTransport, NoticesRemoteDisconnect)
{
    using namespace testing;
    mir::client::rpc::AsioSocketTransport transport{transport_fd};
    auto observer = std::make_shared<NiceMock<MockObserver>>();
    auto done = std::make_shared<mir::test::Signal>();

    ON_CALL(*observer, on_disconnected())
        .WillByDefault(Invoke([done]() { done->raise(); }));

    transport.register_observer(observer);

    close(test_fd);

    EXPECT_TRUE(done->wait_for(std::chrono::milliseconds{100}));
}

TEST_F(AsioSocketTransport, NotifiesOnDataAvailable)
{
    using namespace testing;

    mir::client::rpc::AsioSocketTransport transport{transport_fd};
    auto observer = std::make_shared<NiceMock<MockObserver>>();
    auto done = std::make_shared<mir::test::Signal>();

    ON_CALL(*observer, on_data_available())
        .WillByDefault(Invoke([done]() { done->raise(); }));

    transport.register_observer(observer);

    uint64_t dummy{0xdeadbeef};
    EXPECT_EQ(sizeof(dummy), write(test_fd, &dummy, sizeof(dummy)));

    EXPECT_TRUE(done->wait_for(std::chrono::milliseconds{100}));
}

TEST_F(AsioSocketTransport, DoesntNotifyUntilDataAvailable)
{
    using namespace testing;

    mir::client::rpc::AsioSocketTransport transport{transport_fd};
    auto observer = std::make_shared<NiceMock<MockObserver>>();
    auto done = std::make_shared<mir::test::Signal>();

    ON_CALL(*observer, on_data_available())
        .WillByDefault(Invoke([done]() { done->raise(); }));

    transport.register_observer(observer);

    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    EXPECT_FALSE(done->raised());

    uint64_t dummy{0xdeadbeef};
    EXPECT_EQ(sizeof(dummy), write(test_fd, &dummy, sizeof(dummy)));

    EXPECT_TRUE(done->wait_for(std::chrono::milliseconds{100}));
}

TEST_F(AsioSocketTransport, KeepsNotifyingOfAvailableDataUntilAllIsRead)
{
    using namespace testing;

    mir::client::rpc::AsioSocketTransport transport{transport_fd};
    auto observer = std::make_shared<NiceMock<MockObserver>>();
    auto done = std::make_shared<mir::test::Signal>();

    std::array<uint8_t, sizeof(int) * 256> data;
    data.fill(0);
    std::atomic<size_t> bytes_left{data.size()};

    ON_CALL(*observer, on_data_available())
        .WillByDefault(Invoke([done, &bytes_left, &transport]()
    {
        int dummy;
        transport.receive_data(&dummy, sizeof(dummy));
        bytes_left.fetch_sub(sizeof(dummy));
        if (bytes_left.load() == 0)
        {
            done->raise();
        }
    }));

    transport.register_observer(observer);

    ASSERT_EQ(data.size(), write(test_fd, data.data(), data.size()));

    EXPECT_TRUE(done->wait_for(std::chrono::seconds{5}));
    EXPECT_EQ(0, bytes_left.load());
}

TEST_F(AsioSocketTransport, ReadsCorrectData)
{
    using namespace testing;

    mir::client::rpc::AsioSocketTransport transport{transport_fd};
    auto observer = std::make_shared<NiceMock<MockObserver>>();
    auto done = std::make_shared<mir::test::Signal>();

    std::string expected{"I am the very model of a modern major general"};
    std::vector<char> received(expected.size());

    ON_CALL(*observer, on_data_available())
        .WillByDefault(Invoke([done, &received, &transport]()
    {
        transport.receive_data(received.data(), received.size());
        done->raise();
    }));

    transport.register_observer(observer);

    EXPECT_EQ(expected.size(), write(test_fd, expected.data(), expected.size()));

    ASSERT_TRUE(done->wait_for(std::chrono::milliseconds{100}));
    EXPECT_EQ(0, memcmp(expected.data(), received.data(), expected.size()));
}

TEST_F(AsioSocketTransport, WritesCorrectData)
{
    using namespace testing;

    mir::client::rpc::AsioSocketTransport transport{transport_fd};
    auto observer = std::make_shared<NiceMock<MockObserver>>();
    auto done = std::make_shared<mir::test::Signal>();

    std::string expected{"I am the very model of a modern major general"};
    std::vector<uint8_t> send_buffer{expected.begin(), expected.end()};
    std::vector<char> received(expected.size());

    transport.send_data(send_buffer);

    pollfd read_listener;
    read_listener.fd = test_fd;
    read_listener.events = POLLIN;

    ASSERT_EQ(1, poll(&read_listener, 1, 1000)) << "Failed to poll(): " << strerror(errno);

    EXPECT_EQ(expected.size(), read(test_fd, received.data(), received.size()));
    EXPECT_EQ(0, memcmp(expected.data(), received.data(), expected.size()));
}
