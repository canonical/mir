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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_display.h"
#include "mir/test/doubles/null_platform.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/scene/buffer_stream_factory.h"
#include "mir/scene/session_coordinator.h"
#include "mir/frontend/event_sink.h"
#include "mir/compositor/buffer_stream.h"
#include "src/server/compositor/buffer_bundle.h"
#include "src/server/compositor/buffer_stream_surfaces.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/debug/surface.h"
#include "src/client/mir_connection.h"
#include "src/client/rpc/mir_display_server.h"
#include <chrono>
#include <mutex>
#include <stdio.h>
#include <condition_variable>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mir/fd.h"

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace msc = mir::scene;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;
namespace mf = mir::frontend;
namespace mclr = mir::client::rpc;

namespace
{
MATCHER(DidNotTimeOut, "did not time out")
{
    return arg;
}

struct StubBundle : public mc::BufferBundle
{
    StubBundle(std::vector<mg::BufferID> const& ids) :
        buffer_id_seq(ids)
    {
    }

    void client_acquire(std::function<void(mg::Buffer* buffer)> complete)
    {
        std::shared_ptr<mg::Buffer> stub_buffer;
        if (buffers_acquired < buffer_id_seq.size())
            stub_buffer = std::make_shared<mtd::StubBuffer>(buffer_id_seq.at(buffers_acquired++));
        else
            stub_buffer = std::make_shared<mtd::StubBuffer>(buffer_id_seq.back());

        client_buffers.push_back(stub_buffer);
        complete(stub_buffer.get());
    }
    void client_release(mg::Buffer*) {}
    std::shared_ptr<mg::Buffer> compositor_acquire(void const*)
        { return std::make_shared<mtd::StubBuffer>(); }
    void compositor_release(std::shared_ptr<mg::Buffer> const&) {}
    std::shared_ptr<mg::Buffer> snapshot_acquire()
        { return std::make_shared<mtd::StubBuffer>(); }
    void snapshot_release(std::shared_ptr<mg::Buffer> const&) {}
    mg::BufferProperties properties() const { return mg::BufferProperties{}; }
    void allow_framedropping(bool) {}
    void force_requests_to_complete() {}
    void resize(const geom::Size&) {}
    int buffers_ready_for_compositor(void const*) const { return 1; }
    int buffers_free_for_client() const { return 1; }
    void drop_old_buffers() {}
    void drop_client_requests() {}

    std::vector<std::shared_ptr<mg::Buffer>> client_buffers;
    std::vector<mg::BufferID> const buffer_id_seq;
    unsigned int buffers_acquired{0};
};

struct StubBundleFactory : public msc::BufferStreamFactory
{
    StubBundleFactory(std::vector<mg::BufferID> const& ids) :
        buffer_id_seq(ids)
    {}

    std::shared_ptr<mc::BufferStream> create_buffer_stream(
        mf::BufferStreamId i, std::shared_ptr<mf::BufferSink> const& s,
        int, mg::BufferProperties const& p) override
    { return create_buffer_stream(i, s, p); }
    std::shared_ptr<mc::BufferStream> create_buffer_stream(
        mf::BufferStreamId, std::shared_ptr<mf::BufferSink> const&, mg::BufferProperties const&) override
    { return std::make_shared<mc::BufferStreamSurfaces>(std::make_shared<StubBundle>(buffer_id_seq)); }
    std::vector<mg::BufferID> const buffer_id_seq;
};

struct StubBufferPacker : public mg::PlatformIpcOperations
{
    StubBufferPacker() :
        last_fd{-1}
    {
    }
    void pack_buffer(mg::BufferIpcMessage&, mg::Buffer const&, mg::BufferIpcMsgType) const override
    {
    }

    void unpack_buffer(mg::BufferIpcMessage& msg, mg::Buffer const&) const override
    {
        auto fds = msg.fds();
        if (!fds.empty())
        {
            last_fd = fds[0];
        }
    }

    std::shared_ptr<mg::PlatformIPCPackage> connection_ipc_package() override
    {
        return std::make_shared<mg::PlatformIPCPackage>();
    }

    mir::Fd last_unpacked_fd()
    {
        return last_fd;
    }

    mg::PlatformOperationMessage platform_operation(
        unsigned int const, mg::PlatformOperationMessage const&) override
    {
        return mg::PlatformOperationMessage();
    }
private:
    mir::Fd mutable last_fd;
};

struct StubPlatform : public mtd::NullPlatform
{
    StubPlatform(std::shared_ptr<mg::PlatformIpcOperations> const& ipc_ops) :
        ipc_ops{ipc_ops}
    {
    }

    std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator() override
    {
        return std::make_shared<mtd::StubBufferAllocator>();
    }

    std::shared_ptr<mg::PlatformIpcOperations> make_ipc_operations() const override
    {
        return ipc_ops;
    }

    std::shared_ptr<mg::Display> create_display(
        std::shared_ptr<mg::DisplayConfigurationPolicy> const&,
        std::shared_ptr<mg::GLConfig> const&) override
    {
        std::vector<geom::Rectangle> rect{geom::Rectangle{{0,0},{1,1}}};
        return std::make_shared<mtd::StubDisplay>(rect);
    }
    
    std::shared_ptr<mg::PlatformIpcOperations> const ipc_ops;
};

class SinkSkimmingCoordinator : public msc::SessionCoordinator
{
public:
    SinkSkimmingCoordinator(std::shared_ptr<msc::SessionCoordinator> const& wrapped) :
        wrapped(wrapped)
    {
    }

    void set_focus_to(std::shared_ptr<msc::Session> const& focus) override
    {
        wrapped->set_focus_to(focus);
    }

    void unset_focus() override
    {
        wrapped->unset_focus();
    }

    std::shared_ptr<msc::Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<mf::EventSink> const& sink) override
    {
        last_sink = sink;
        return wrapped->open_session(client_pid, name, sink);
    }

    void close_session(std::shared_ptr<msc::Session> const& session) override
    {
        wrapped->close_session(session);
    }

    std::shared_ptr<msc::Session> successor_of(std::shared_ptr<msc::Session> const& session) const override
    {
        return wrapped->successor_of(session);
    }

    std::shared_ptr<msc::SessionCoordinator> const wrapped;
    std::weak_ptr<mf::EventSink> last_sink;
};

struct ExchangeServerConfiguration : mtf::StubbedServerConfiguration
{
    ExchangeServerConfiguration(
        std::vector<mg::BufferID> const& id_seq,
        std::shared_ptr<mg::PlatformIpcOperations> const& ipc_ops) :
        stream_factory{std::make_shared<StubBundleFactory>(id_seq)},
        platform{std::make_shared<StubPlatform>(ipc_ops)}
    {
    }

    std::shared_ptr<msc::SessionCoordinator> the_session_coordinator() override
    {
        return session_coordinator([this]{
            coordinator = std::make_shared<SinkSkimmingCoordinator>(
                DefaultServerConfiguration::the_session_coordinator());
            return coordinator;
        });
    }
    std::shared_ptr<mg::Platform> the_graphics_platform() override
    {
        return platform;
    }

    std::shared_ptr<msc::BufferStreamFactory> the_buffer_stream_factory() override
    {
        return stream_factory;
    }

    std::shared_ptr<StubBundleFactory> const stream_factory;
    std::shared_ptr<mg::Platform> const platform;
    std::shared_ptr<SinkSkimmingCoordinator> coordinator;
};

struct ExchangeBufferTest : mir_test_framework::InProcessServer
{
    std::vector<mg::BufferID> const buffer_id_exchange_seq{
        mg::BufferID{4}, mg::BufferID{8}, mg::BufferID{9}, mg::BufferID{3}, mg::BufferID{4}};

    std::shared_ptr<StubBufferPacker> stub_packer{std::make_shared<StubBufferPacker>()};
    ExchangeServerConfiguration server_configuration{buffer_id_exchange_seq, stub_packer};
    mir::DefaultServerConfiguration& server_config() override { return server_configuration; }
    mtf::UsingStubClientPlatform using_stub_client_platform;

    void request_completed()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        arrived = true;
        cv.notify_all();
    }

    bool exchange_buffer(mclr::DisplayServer& server)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        mp::Buffer next;
        server.exchange_buffer(&buffer_request, &next,
            google::protobuf::NewCallback(this, &ExchangeBufferTest::request_completed));


        arrived = false;
        auto completed = cv.wait_for(lk, std::chrono::seconds(5), [this]() {return arrived;});
        for (auto i = 0; i < next.fd().size(); i++)
            ::close(next.fd(i));
        next.set_fds_on_side_channel(0);

        *buffer_request.mutable_buffer() = next;
        return completed;
    }

    bool submit_buffer(mclr::DisplayServer& server, mp::BufferRequest& request)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        mp::Void v;
        server.submit_buffer(&request, &v,
            google::protobuf::NewCallback(this, &ExchangeBufferTest::request_completed));
        
        arrived = false;
        return cv.wait_for(lk, std::chrono::seconds(5), [this]() {return arrived;});
    }

    bool allocate_buffers(mclr::DisplayServer& server, mp::BufferAllocation& request)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        mp::Void v;
        server.allocate_buffers(&request, &v,
            google::protobuf::NewCallback(this, &ExchangeBufferTest::request_completed));
        
        arrived = false;
        return cv.wait_for(lk, std::chrono::seconds(5), [this]() {return arrived;});
    }

    bool release_buffers(mclr::DisplayServer& server, mp::BufferRelease& request)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        mp::Void v;
        server.release_buffers(&request, &v,
            google::protobuf::NewCallback(this, &ExchangeBufferTest::request_completed));
        
        arrived = false;
        return cv.wait_for(lk, std::chrono::seconds(5), [this]() {return arrived;});
    }

    std::mutex mutex;
    std::condition_variable cv;
    bool arrived{false};
    mp::BufferRequest buffer_request; 
};
}

//tests for the exchange_buffer rpc call
TEST_F(ExchangeBufferTest, exchanges_happen)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto surface = mtf::make_any_surface(connection);

    auto rpc_channel = connection->rpc_channel();
    mclr::DisplayServer server(rpc_channel);
    buffer_request.mutable_buffer()->set_buffer_id(buffer_id_exchange_seq.begin()->as_value());
    for (auto i = 0; i < buffer_request.buffer().fd().size(); i++)
        ::close(buffer_request.buffer().fd(i));

    for (auto const& id : buffer_id_exchange_seq)
    {
        EXPECT_THAT(buffer_request.buffer().buffer_id(), testing::Eq(id.as_value()));
        ASSERT_THAT(exchange_buffer(server), DidNotTimeOut());
    }

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

namespace
{
MATCHER(NoErrorOnFileRead, "")
{
    return arg > 0;
}
}
TEST_F(ExchangeBufferTest, fds_can_be_sent_back)
{
    using namespace testing;
    std::string test_string{"mir was a space station"};
    mir::Fd file(fileno(tmpfile()));
    EXPECT_THAT(write(file, test_string.c_str(), test_string.size()), Gt(0));

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto surface = mtf::make_any_surface(connection);

    auto rpc_channel = connection->rpc_channel();
    mclr::DisplayServer server(rpc_channel);
    for (auto i = 0; i < buffer_request.buffer().fd().size(); i++)
        ::close(buffer_request.buffer().fd(i));

    buffer_request.mutable_buffer()->set_buffer_id(buffer_id_exchange_seq.begin()->as_value());
    buffer_request.mutable_buffer()->add_fd(file);

    ASSERT_THAT(exchange_buffer(server), DidNotTimeOut());

    mir_surface_release_sync(surface);
    mir_connection_release(connection);

    auto server_received_fd = stub_packer->last_unpacked_fd();
    char file_buffer[32];
    lseek(file, 0, SEEK_SET);
    ASSERT_THAT(read(server_received_fd, file_buffer, sizeof(file_buffer)), NoErrorOnFileRead());
    EXPECT_THAT(strncmp(test_string.c_str(), file_buffer, test_string.size()), Eq(0)); 
}

//tests for the submit buffer protocol.
TEST_F(ExchangeBufferTest, submissions_happen)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto surface = mtf::make_any_surface(connection);

    auto rpc_channel = connection->rpc_channel();
    mclr::DisplayServer server(rpc_channel);

    mp::BufferRequest request;
    for (auto const& id : buffer_id_exchange_seq)
    {
        buffer_request.mutable_buffer()->set_buffer_id(id.as_value());
        ASSERT_THAT(submit_buffer(server, buffer_request), DidNotTimeOut());
    }

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

namespace
{
template<class Clock>
bool spin_wait_for_id(mg::BufferID id, MirSurface* surface, std::chrono::time_point<Clock> const& pt)
{
    while(Clock::now() < pt)
    {
        if (mir_debug_surface_current_buffer_id(surface) == id.as_value())
            return true;
        std::this_thread::yield();
    }
    return false;
}
}

TEST_F(ExchangeBufferTest, server_can_send_buffer)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto surface = mtf::make_any_surface(connection);
    auto sink = server_configuration.coordinator->last_sink.lock();

    //first wait for the last id in the exchange sequence to be seen. Avoids lp: #1487967, where
    //the stub sink could send before the server sends its sequence.
    auto timeout = std::chrono::steady_clock::now() + 5s;
    EXPECT_TRUE(spin_wait_for_id(buffer_id_exchange_seq.back(), surface, timeout))
        << "failed to see the last scheduled buffer become the current one";

    //spin-wait for the id to become the current one.
    //The notification doesn't generate a client-facing callback on the stream yet
    //(although probably should, seems like something a media decoder would need)
    mtd::StubBuffer stub_buffer;
    timeout = std::chrono::steady_clock::now() + 5s;
    sink->send_buffer(mf::BufferStreamId{0}, stub_buffer, mg::BufferIpcMsgType::full_msg);
    EXPECT_TRUE(spin_wait_for_id(stub_buffer.id(), surface, timeout))
        << "failed to see the sent buffer become the current one";

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

//TODO: check that a buffer arrives asynchronously.
TEST_F(ExchangeBufferTest, allocate_buffers_doesnt_time_out)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto surface = mtf::make_any_surface(connection);

    auto rpc_channel = connection->rpc_channel();
    mclr::DisplayServer server(rpc_channel);

    mp::BufferAllocation request;
    EXPECT_THAT(allocate_buffers(server, request), DidNotTimeOut());

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

TEST_F(ExchangeBufferTest, release_buffers_doesnt_time_out)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto surface = mtf::make_any_surface(connection);

    auto rpc_channel = connection->rpc_channel();
    mclr::DisplayServer server(rpc_channel);

    mp::BufferRelease request;
    EXPECT_THAT(release_buffers(server, request), DidNotTimeOut());

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}
