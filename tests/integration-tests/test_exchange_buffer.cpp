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
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/stub_display.h"
#include "mir_test_doubles/null_platform.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/scene/buffer_stream_factory.h"
#include "mir/compositor/buffer_stream.h"
#include "mir_toolkit/mir_client_library.h"
#include "src/client/mir_connection.h"
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

namespace
{
MATCHER(DidNotTimeOut, "did not time out")
{
    return arg;
}

struct StubStream : public mc::BufferStream
{
    StubStream(std::vector<mg::BufferID> const& ids) :
        buffer_id_seq(ids)
    {
    }

    void acquire_client_buffer(std::function<void(mg::Buffer* buffer)> complete)
    {
        std::shared_ptr<mg::Buffer> stub_buffer;
        if (buffers_acquired < buffer_id_seq.size())
            stub_buffer = std::make_shared<mtd::StubBuffer>(buffer_id_seq.at(buffers_acquired++));
        else
            stub_buffer = std::make_shared<mtd::StubBuffer>(buffer_id_seq.back());

        client_buffers.push_back(stub_buffer);
        complete(stub_buffer.get());
    }

    void release_client_buffer(mg::Buffer*) {}
    std::shared_ptr<mg::Buffer> lock_compositor_buffer(void const*)
    { return std::make_shared<mtd::StubBuffer>(); }
    std::shared_ptr<mg::Buffer> lock_snapshot_buffer()
    { return std::make_shared<mtd::StubBuffer>(); }
    MirPixelFormat get_stream_pixel_format() { return mir_pixel_format_abgr_8888; }
    geom::Size stream_size() { return geom::Size{1,212121}; }
    void resize(geom::Size const&) {};
    void allow_framedropping(bool) {};
    void force_requests_to_complete() {};
    int buffers_ready_for_compositor() const { return -5; }
    void drop_old_buffers() {}
    void drop_client_requests() override {}

    std::vector<std::shared_ptr<mg::Buffer>> client_buffers;
    std::vector<mg::BufferID> const buffer_id_seq;
    unsigned int buffers_acquired{0};
};

struct StubStreamFactory : public msc::BufferStreamFactory
{
    StubStreamFactory(std::vector<mg::BufferID> const& ids) :
        buffer_id_seq(ids)
    {}

    std::shared_ptr<mc::BufferStream> create_buffer_stream(mg::BufferProperties const&) override
    { return std::make_shared<StubStream>(buffer_id_seq); }
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
        std::shared_ptr<mg::GLProgramFactory> const&,
        std::shared_ptr<mg::GLConfig> const&) override
    {
        std::vector<geom::Rectangle> rect{geom::Rectangle{{0,0},{1,1}}};
        return std::make_shared<mtd::StubDisplay>(rect);
    }
    
    std::shared_ptr<mg::BufferWriter> make_buffer_writer() override
    {
        return nullptr;
    }

    std::shared_ptr<mg::PlatformIpcOperations> const ipc_ops;
};

struct ExchangeServerConfiguration : mtf::StubbedServerConfiguration
{
    ExchangeServerConfiguration(
        std::vector<mg::BufferID> const& id_seq,
        std::shared_ptr<mg::PlatformIpcOperations> const& ipc_ops) :
        stream_factory{std::make_shared<StubStreamFactory>(id_seq)},
        platform{std::make_shared<StubPlatform>(ipc_ops)}
    {
    }

    std::shared_ptr<mg::Platform> the_graphics_platform() override
    {
        return platform;
    }

    std::shared_ptr<msc::BufferStreamFactory> the_buffer_stream_factory() override
    {
        return stream_factory;
    }

    std::shared_ptr<msc::BufferStreamFactory> const stream_factory;
    std::shared_ptr<mg::Platform> const platform;
};

struct ExchangeBufferTest : mir_test_framework::InProcessServer
{
    std::vector<mg::BufferID> const buffer_id_exchange_seq{
        mg::BufferID{4}, mg::BufferID{8}, mg::BufferID{9}, mg::BufferID{3}, mg::BufferID{4}};

    std::shared_ptr<StubBufferPacker> stub_packer{std::make_shared<StubBufferPacker>()};
    ExchangeServerConfiguration server_configuration{buffer_id_exchange_seq, stub_packer};
    mir::DefaultServerConfiguration& server_config() override { return server_configuration; }
    mtf::UsingStubClientPlatform using_stub_client_platform;

    void buffer_arrival()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        arrived = true;
        cv.notify_all();
    }

    //TODO: once the next_buffer rpc is deprecated, change this code out for the
    //      mir_surface_next_buffer() api call
    bool exchange_buffer(mp::DisplayServer& server)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        mp::Buffer next;
        server.exchange_buffer(0, &buffer_request, &next,
            google::protobuf::NewCallback(this, &ExchangeBufferTest::buffer_arrival));


        arrived = false;
        auto completed = cv.wait_for(lk, std::chrono::seconds(5), [this]() {return arrived;});
        for(auto i = 0; i < next.fd().size(); i++)
            ::close(next.fd(i));
        next.set_fds_on_side_channel(0);

        *buffer_request.mutable_buffer() = next;
        return completed;
    }

    std::mutex mutex;
    std::condition_variable cv;
    bool arrived{false};
    mp::BufferRequest buffer_request; 
};
}

TEST_F(ExchangeBufferTest, exchanges_happen)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };
    auto surface = mir_connection_create_surface_sync(connection, &request_params);
    auto rpc_channel = connection->rpc_channel();
    mp::DisplayServer::Stub server(
        rpc_channel.get(), ::google::protobuf::Service::STUB_DOESNT_OWN_CHANNEL);
    buffer_request.mutable_buffer()->set_buffer_id(buffer_id_exchange_seq.begin()->as_value());
    for(auto i = 0; i < buffer_request.buffer().fd().size(); i++)
        ::close(buffer_request.buffer().fd(i));

    for(auto const& id : buffer_id_exchange_seq)
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
    MirSurfaceParameters const request_params =
    {
        __PRETTY_FUNCTION__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };
    auto surface = mir_connection_create_surface_sync(connection, &request_params);
    auto rpc_channel = connection->rpc_channel();
    mp::DisplayServer::Stub server(
            rpc_channel.get(), ::google::protobuf::Service::STUB_DOESNT_OWN_CHANNEL);
    for(auto i = 0; i < buffer_request.buffer().fd().size(); i++)
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
