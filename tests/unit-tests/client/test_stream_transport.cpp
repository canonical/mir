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
#include "src/client/rpc/stream_socket_transport.h"

#include "mir_test/auto_unblock_thread.h"
#include "mir_test/signal.h"
#include "mir/raii.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <cstdint>
#include <thread>
#include <system_error>
#include <array>
#include <atomic>
#include <poll.h>
#include <signal.h>
#include <sys/time.h>

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

template<typename TransportMechanism>
class StreamTransportTest : public ::testing::Test
{
public:
    StreamTransportTest()
    {
        int socket_fds[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds) < 0)
        {
            throw std::system_error(errno, std::system_category());
        }

        test_fd = socket_fds[0];
        transport_fd = socket_fds[1];
        transport = new TransportMechanism(socket_fds[1]);
    }

    virtual ~StreamTransportTest()
    {
        delete transport;
        // Transport should have closed its fd

        // We don't care about errors, so unconditionally close the test fd.
        close(test_fd);
    }

    int transport_fd;
    int test_fd;
    TransportMechanism* transport;
};

typedef ::testing::Types<mclr::StreamSocketTransport> Transports;
TYPED_TEST_CASE(StreamTransportTest, Transports);

TYPED_TEST(StreamTransportTest, NoticesRemoteDisconnect)
{
    using namespace testing;
    auto observer = std::make_shared<NiceMock<MockObserver>>();
    auto done = std::make_shared<mir::test::Signal>();

    ON_CALL(*observer, on_disconnected())
        .WillByDefault(Invoke([done]() { done->raise(); }));

    this->transport->register_observer(observer);

    close(this->test_fd);

    EXPECT_TRUE(done->wait_for(std::chrono::milliseconds{100}));
}

TYPED_TEST(StreamTransportTest, NotifiesOnDataAvailable)
{
    using namespace testing;

    auto observer = std::make_shared<NiceMock<MockObserver>>();
    auto done = std::make_shared<mir::test::Signal>();

    ON_CALL(*observer, on_data_available())
        .WillByDefault(Invoke([done]() { done->raise(); }));

    this->transport->register_observer(observer);

    uint64_t dummy{0xdeadbeef};
    EXPECT_EQ(sizeof(dummy), write(this->test_fd, &dummy, sizeof(dummy)));

    EXPECT_TRUE(done->wait_for(std::chrono::milliseconds{100}));
}

TYPED_TEST(StreamTransportTest, DoesntNotifyUntilDataAvailable)
{
    using namespace testing;

    auto observer = std::make_shared<NiceMock<MockObserver>>();
    auto done = std::make_shared<mir::test::Signal>();

    ON_CALL(*observer, on_data_available())
        .WillByDefault(Invoke([done]() { done->raise(); }));

    this->transport->register_observer(observer);

    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    EXPECT_FALSE(done->raised());

    uint64_t dummy{0xdeadbeef};
    EXPECT_EQ(sizeof(dummy), write(this->test_fd, &dummy, sizeof(dummy)));

    EXPECT_TRUE(done->wait_for(std::chrono::milliseconds{100}));
}

TYPED_TEST(StreamTransportTest, KeepsNotifyingOfAvailableDataUntilAllIsRead)
{
    using namespace testing;

    auto observer = std::make_shared<NiceMock<MockObserver>>();
    auto done = std::make_shared<mir::test::Signal>();

    std::array<uint8_t, sizeof(int) * 256> data;
    data.fill(0);
    std::atomic<size_t> bytes_left{data.size()};

    ON_CALL(*observer, on_data_available())
        .WillByDefault(Invoke([done, &bytes_left, this]()
    {
        int dummy;
        this->transport->receive_data(&dummy, sizeof(dummy));
        bytes_left.fetch_sub(sizeof(dummy));
        if (bytes_left.load() == 0)
        {
            done->raise();
        }
    }));

    this->transport->register_observer(observer);

    EXPECT_EQ(data.size(), write(this->test_fd, data.data(), data.size()));

    EXPECT_TRUE(done->wait_for(std::chrono::seconds{5}));
    EXPECT_EQ(0, bytes_left.load());
}

TYPED_TEST(StreamTransportTest, ReadsCorrectData)
{
    using namespace testing;

    auto observer = std::make_shared<NiceMock<MockObserver>>();
    auto done = std::make_shared<mir::test::Signal>();

    std::string expected{"I am the very model of a modern major general"};
    std::vector<char> received(expected.size());

    ON_CALL(*observer, on_data_available())
        .WillByDefault(Invoke([done, &received, this]()
    {
        this->transport->receive_data(received.data(), received.size());
        done->raise();
    }));

    this->transport->register_observer(observer);

    EXPECT_EQ(expected.size(), write(this->test_fd, expected.data(), expected.size()));

    ASSERT_TRUE(done->wait_for(std::chrono::milliseconds{100}));
    EXPECT_EQ(0, memcmp(expected.data(), received.data(), expected.size()));
}

TYPED_TEST(StreamTransportTest, WritesCorrectData)
{
    using namespace testing;

    auto observer = std::make_shared<NiceMock<MockObserver>>();
    auto done = std::make_shared<mir::test::Signal>();

    std::string expected{"I am the very model of a modern major general"};
    std::vector<uint8_t> send_buffer{expected.begin(), expected.end()};
    std::vector<char> received(expected.size());

    this->transport->send_data(send_buffer);

    pollfd read_listener;
    read_listener.fd = this->test_fd;
    read_listener.events = POLLIN;

    ASSERT_EQ(1, poll(&read_listener, 1, 1000)) << "Failed to poll(): " << strerror(errno);

    EXPECT_EQ(expected.size(), read(this->test_fd, received.data(), received.size()));
    EXPECT_EQ(0, memcmp(expected.data(), received.data(), expected.size()));
}

namespace
{
bool alarm_raised;
void set_alarm_raised(int /*signo*/)
{
    alarm_raised = true;
}

}

TYPED_TEST(StreamTransportTest, ReadsFullDataFromMultipleChunks)
{
    size_t const chunk_size{8};
    std::vector<uint8_t> expected(chunk_size * 4);

    uint8_t counter{0};
    for (auto& byte : expected)
    {
        byte = counter++;
    }
    std::vector<uint8_t> received(expected.size());

    auto reader = std::thread([&]()
    {
        this->transport->receive_data(received.data(), received.size());
    });

    size_t bytes_written{0};
    while (bytes_written < expected.size())
    {
        auto result = send(this->test_fd,
                           expected.data() + bytes_written,
                           std::min(chunk_size, expected.size() - bytes_written),
                           MSG_DONTWAIT);

        ASSERT_NE(-1, result) << "Failed to send(): " << strerror(errno);
        bytes_written += result;
    }

    if (reader.joinable())
    {
        reader.join();
    }
    EXPECT_EQ(expected, received);
}

TYPED_TEST(StreamTransportTest, ReadsFullDataWhenInterruptedWithSignals)
{
    size_t const chunk_size{262144};
    std::vector<uint8_t> expected(chunk_size * 4);

    uint8_t counter{0};
    for (auto& byte : expected)
    {
        byte = counter++;
    }
    std::vector<uint8_t> received(expected.size());

    struct sigaction old_handler;

    auto sig_alarm_handler = mir::raii::paired_calls([&old_handler]()
    {
        struct sigaction alarm_handler;
        sigset_t blocked_signals;

        sigemptyset(&blocked_signals);
        alarm_handler.sa_handler = &set_alarm_raised;
        alarm_handler.sa_flags = 0;
        alarm_handler.sa_mask = blocked_signals;
        if (sigaction(SIGALRM, &alarm_handler, &old_handler) < 0)
        {
            throw std::system_error{errno, std::system_category(), "Failed to set SIGALRM handler"};
        }
    },
    [&old_handler]()
    {
        if (sigaction(SIGALRM, &old_handler, nullptr) < 0)
        {
            throw std::system_error{errno, std::system_category(), "Failed to restore SIGALRM handler"};
        }
    });


    auto reader = std::thread([&]()
    {
        alarm_raised = false;
        this->transport->receive_data(received.data(), received.size());
        EXPECT_TRUE(alarm_raised);
    });

    size_t bytes_written{0};
    while (bytes_written < expected.size())
    {
        pollfd socket_writable;
        socket_writable.events = POLLOUT;
        socket_writable.fd = this->test_fd;

        ASSERT_GE(poll(&socket_writable, 1, 10000), 1);
        ASSERT_EQ(0, socket_writable.revents & (POLLERR | POLLHUP));

        auto result = send(this->test_fd,
                           expected.data() + bytes_written,
                           std::min(chunk_size, expected.size() - bytes_written),
                           MSG_DONTWAIT);

        ASSERT_NE(-1, result) << "Failed to send(): " << strerror(errno);
        bytes_written += result;
        pthread_kill(reader.native_handle(), SIGALRM);
    }

    if (reader.joinable())
    {
        reader.join();
    }

    EXPECT_EQ(expected, received);
}
