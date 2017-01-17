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
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_frame_dropping_policy_factory.h"
#include "mir/test/doubles/stub_display.h"
#include "mir/test/doubles/null_platform.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/platform_operation_message.h"
#include "mir/scene/buffer_stream_factory.h"
#include "mir/scene/session_coordinator.h"
#include "mir/frontend/event_sink.h"
#include "mir/compositor/buffer_stream.h"
#include "src/server/compositor/stream.h"
#include "src/server/compositor/buffer_map.h"
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

struct StubStreamFactory : public msc::BufferStreamFactory
{
    StubStreamFactory(std::vector<mg::BufferID> const& ids) :
        buffer_id_seq(ids)
    {}

    std::shared_ptr<mc::BufferStream> create_buffer_stream(
        mf::BufferStreamId i, std::shared_ptr<mf::ClientBuffers> const& s,
        int, mg::BufferProperties const& p) override
    {
        return create_buffer_stream(i, s, p);
    }

    std::shared_ptr<mc::BufferStream> create_buffer_stream(
        mf::BufferStreamId, std::shared_ptr<mf::ClientBuffers> const& sink,
        mg::BufferProperties const& properties) override
    {
        return std::make_shared<mc::Stream>(factory, sink, properties.size, properties.format);
    }

    std::shared_ptr<mf::ClientBuffers> create_buffer_map(std::shared_ptr<mf::BufferSink> const& sink) override
    {
        struct BufferMap : mf::ClientBuffers
        {
            BufferMap(
                std::shared_ptr<mf::BufferSink> const& sink,
                std::vector<mg::BufferID> const& ids) :
                sink(sink),
                buffer_id_seq(ids)
            {
                std::reverse(buffer_id_seq.begin(), buffer_id_seq.end());
            }

            mg::BufferID add_buffer(std::shared_ptr<mg::Buffer> const& buffer) override
            {
                struct BufferIdWrapper : mg::Buffer
                {
                    BufferIdWrapper(std::shared_ptr<mg::Buffer> const& b, mg::BufferID id) :
                        wrapped(b),
                        id_(id)
                    {
                    }
                
                    std::shared_ptr<mg::NativeBuffer> native_buffer_handle() const override
                    {
                        return wrapped->native_buffer_handle();
                    }
                    mg::BufferID id() const override
                    {
                        return id_;
                    }
                    geom::Size size() const override
                    {
                        return wrapped->size();
                    }
                    MirPixelFormat pixel_format() const override
                    {
                        return wrapped->pixel_format();
                    }
                    mg::NativeBufferBase* native_buffer_base() override
                    {
                        return wrapped->native_buffer_base();
                    }
                    std::shared_ptr<mg::Buffer> const wrapped;
                    mg::BufferID const id_;
                };

                auto id = buffer_id_seq.back();
                buffer_id_seq.pop_back();
                b = std::make_shared<BufferIdWrapper>(buffer, id);
                sink->add_buffer(*b);
                return id; 
            }

            void remove_buffer(mg::BufferID) override
            {
            } 

            std::shared_ptr<mg::Buffer>& operator[](mg::BufferID) override
            {
                return b;
            }

            void send_buffer(mg::BufferID) override
            {
            }

            void receive_buffer(mg::BufferID) override
            {
            }

            std::shared_ptr<mg::Buffer> b;
            std::shared_ptr<mf::BufferSink> const sink;
            std::shared_ptr<mtd::StubBufferAllocator> alloc{std::make_shared<mtd::StubBufferAllocator>()};
            std::vector<mg::BufferID> buffer_id_seq;
        };
        return std::make_shared<BufferMap>(sink, buffer_id_seq);
    }

    std::vector<mg::BufferID> const buffer_id_seq;
    mtd::StubFrameDroppingPolicyFactory factory;
};

struct StubBufferPacker : public mg::PlatformIpcOperations
{
    StubBufferPacker(
        std::shared_ptr<mir::Fd> const& last_fd,
        std::shared_ptr<mg::PlatformIpcOperations> const& underlying_ops) :
        last_fd{last_fd},
        underlying_ops{underlying_ops}
    {
        *last_fd = mir::Fd{-1};
    }
    void pack_buffer(mg::BufferIpcMessage& msg, mg::Buffer const& b, mg::BufferIpcMsgType) const override
    {
        msg.pack_size(b.size());
    }

    void unpack_buffer(mg::BufferIpcMessage& msg, mg::Buffer const&) const override
    {
        auto fds = msg.fds();
        if (!fds.empty())
        {
            *last_fd = fds[0];
        }
    }

    std::shared_ptr<mg::PlatformIPCPackage> connection_ipc_package() override
    {
        return underlying_ops->connection_ipc_package();
    }

    mg::PlatformOperationMessage platform_operation(
        unsigned int const opcode, mg::PlatformOperationMessage const& msg) override
    {
        return underlying_ops->platform_operation(opcode, msg);
    }
private:
    std::shared_ptr<mir::Fd> const last_fd;
    std::shared_ptr<mg::PlatformIpcOperations> const underlying_ops;
};

struct StubPlatform : public mg::Platform
{
    StubPlatform(std::shared_ptr<mir::Fd> const& last_fd)
        : last_fd(last_fd),
          underlying_platform{mtf::make_stubbed_server_graphics_platform({geom::Rectangle{{0,0},{1,1}}})}
    {
    }

    mir::UniqueModulePtr<mg::GraphicBufferAllocator> create_buffer_allocator() override
    {
        return underlying_platform->create_buffer_allocator();
    }

    mir::UniqueModulePtr<mg::PlatformIpcOperations> make_ipc_operations() const override
    {
        return mir::make_module_ptr<StubBufferPacker>(
            last_fd,
            underlying_platform->make_ipc_operations());
    }

    mir::UniqueModulePtr<mg::Display> create_display(
        std::shared_ptr<mg::DisplayConfigurationPolicy> const& policy,
        std::shared_ptr<mg::GLConfig> const& config) override
    {
        return underlying_platform->create_display(policy, config);
    }

    std::shared_ptr<mir::Fd> const last_fd;
    std::shared_ptr<mg::Platform> const underlying_platform;
};

struct ExchangeServerConfiguration : mtf::StubbedServerConfiguration
{
    ExchangeServerConfiguration(
        std::vector<mg::BufferID> const& id_seq,
        std::shared_ptr<mir::Fd> const& last_unpacked_fd) :
        stream_factory{std::make_shared<StubStreamFactory>(id_seq)},
        platform{std::make_shared<StubPlatform>(last_unpacked_fd)}
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

    std::shared_ptr<StubStreamFactory> const stream_factory;
    std::shared_ptr<mg::Platform> const platform;
};

struct SubmitBuffer : mir_test_framework::InProcessServer
{
    std::vector<mg::BufferID> const buffer_id_exchange_seq{
        mg::BufferID{4}, mg::BufferID{8}, mg::BufferID{9}, mg::BufferID{3}, mg::BufferID{4}};

    std::shared_ptr<mir::Fd> last_unpacked_fd{std::make_shared<mir::Fd>()};
    ExchangeServerConfiguration server_configuration{buffer_id_exchange_seq, last_unpacked_fd};
    mir::DefaultServerConfiguration& server_config() override { return server_configuration; }

    void request_completed()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        arrived = true;
        cv.notify_all();
    }

    bool submit_buffer(mclr::DisplayServer& server, mp::BufferRequest& request)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        mp::Void v;
        server.submit_buffer(&request, &v,
            google::protobuf::NewCallback(this, &SubmitBuffer::request_completed));
        
        arrived = false;
        return cv.wait_for(lk, std::chrono::seconds(5), [this]() {return arrived;});
    }

    bool allocate_buffers(mclr::DisplayServer& server, mp::BufferAllocation& request)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        mp::Void v;
        server.allocate_buffers(&request, &v,
            google::protobuf::NewCallback(this, &SubmitBuffer::request_completed));
        
        arrived = false;
        return cv.wait_for(lk, std::chrono::seconds(5), [this]() {return arrived;});
    }

    bool release_buffers(mclr::DisplayServer& server, mp::BufferRelease& request)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        mp::Void v;
        server.release_buffers(&request, &v,
            google::protobuf::NewCallback(this, &SubmitBuffer::request_completed));
        
        arrived = false;
        return cv.wait_for(lk, std::chrono::seconds(5), [this]() {return arrived;});
    }

    std::mutex mutex;
    std::condition_variable cv;
    bool arrived{false};
    mp::BufferRequest buffer_request; 
};
template<class Clock>
bool spin_wait_for_id(mg::BufferID id, MirWindow* window, std::chrono::time_point<Clock> const& pt)
{
    while(Clock::now() < pt)
    {
        //auto z = mir_debug_window_current_buffer_id(window);
        if (mir_debug_window_current_buffer_id(window) == id.as_value())
            return true;
        std::this_thread::yield();
    }
    return false;
}
}

namespace
{
MATCHER(NoErrorOnFileRead, "")
{
    return arg > 0;
}
}
TEST_F(SubmitBuffer, fds_can_be_sent_back)
{
    using namespace testing;
    std::string test_string{"mir was a space station"};
    mir::Fd file(fileno(tmpfile()));
    EXPECT_THAT(write(file, test_string.c_str(), test_string.size()), Gt(0));

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto window = mtf::make_any_surface(connection);

    auto rpc_channel = connection->rpc_channel();
    mclr::DisplayServer server(rpc_channel);
    for (auto i = 0; i < buffer_request.buffer().fd().size(); i++)
        ::close(buffer_request.buffer().fd(i));

    buffer_request.mutable_buffer()->set_buffer_id(buffer_id_exchange_seq.begin()->as_value());
    buffer_request.mutable_buffer()->add_fd(file);

    ASSERT_THAT(submit_buffer(server, buffer_request), DidNotTimeOut());

    mir_window_release_sync(window);
    mir_connection_release(connection);

    auto server_received_fd = *last_unpacked_fd;
    char file_buffer[32];
    lseek(file, 0, SEEK_SET);
    ASSERT_THAT(read(server_received_fd, file_buffer, sizeof(file_buffer)), NoErrorOnFileRead());
    EXPECT_THAT(strncmp(test_string.c_str(), file_buffer, test_string.size()), Eq(0)); 
}

//tests for the submit buffer protocol.
TEST_F(SubmitBuffer, submissions_happen)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto window = mtf::make_any_surface(connection);

    auto rpc_channel = connection->rpc_channel();
    mclr::DisplayServer server(rpc_channel);

    mp::BufferRequest request;
    for (auto const& id : buffer_id_exchange_seq)
    {
        buffer_request.mutable_buffer()->set_buffer_id(id.as_value());
        ASSERT_THAT(submit_buffer(server, buffer_request), DidNotTimeOut());
    }

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(SubmitBuffer, server_can_send_buffer)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto window = mtf::make_any_surface(connection);

    auto timeout = std::chrono::steady_clock::now() + 5s;
    EXPECT_TRUE(spin_wait_for_id(buffer_id_exchange_seq.back(), window, timeout))
        << "failed to see the last scheduled buffer become the current one";

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

//TODO: check that a buffer arrives asynchronously.
TEST_F(SubmitBuffer, allocate_buffers_doesnt_time_out)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto window = mtf::make_any_surface(connection);

    auto rpc_channel = connection->rpc_channel();
    mclr::DisplayServer server(rpc_channel);

    mp::BufferAllocation request;
    EXPECT_THAT(allocate_buffers(server, request), DidNotTimeOut());

    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(SubmitBuffer, release_buffers_doesnt_time_out)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto window = mtf::make_any_surface(connection);

    auto rpc_channel = connection->rpc_channel();
    mclr::DisplayServer server(rpc_channel);

    mp::BufferRelease request;
    EXPECT_THAT(release_buffers(server, request), DidNotTimeOut());

    mir_window_release_sync(window);
    mir_connection_release(connection);
}
