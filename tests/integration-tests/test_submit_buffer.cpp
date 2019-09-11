/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
        int, mg::BufferProperties const& p) override
    {
        return create_buffer_stream(p);
    }

    std::shared_ptr<mc::BufferStream> create_buffer_stream(
        mg::BufferProperties const& properties) override
    {
        return std::make_shared<mc::Stream>(properties.size, properties.format);
    }

    std::vector<mg::BufferID> const buffer_id_seq;
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

class RecordingBufferAllocator : public mg::GraphicBufferAllocator
{
public:
    RecordingBufferAllocator(
        std::shared_ptr<mg::GraphicBufferAllocator> const& wrapped,
        std::vector<std::weak_ptr<mg::Buffer>>& allocated_buffers,
        std::mutex& buffer_mutex)
        : allocated_buffers{allocated_buffers},
          underlying_allocator{wrapped},
          buffer_mutex{buffer_mutex}
    {
    }

    std::shared_ptr<mg::Buffer> alloc_buffer(
        mg::BufferProperties const& buffer_properties) override
    {
        auto const buf = underlying_allocator->alloc_buffer(buffer_properties);
        {
            std::lock_guard<std::mutex> lock{buffer_mutex};
            allocated_buffers.push_back(buf);
        }
        return buf;
    }

    std::vector<MirPixelFormat> supported_pixel_formats() override
    {
        return underlying_allocator->supported_pixel_formats();
    }

    std::shared_ptr<mg::Buffer> alloc_buffer(
        geom::Size size,
        uint32_t native_format,
        uint32_t native_flags) override
    {
        auto const buf = underlying_allocator->alloc_buffer(size, native_format, native_flags);
        {
            std::lock_guard<std::mutex> lock{buffer_mutex};
            allocated_buffers.push_back(buf);
        }
        return buf;
    }

    std::shared_ptr<mg::Buffer> alloc_software_buffer(
        geom::Size size,
        MirPixelFormat format) override
    {
        auto const buf = underlying_allocator->alloc_software_buffer(size, format);
        {
            std::lock_guard<std::mutex> lock{buffer_mutex};
            allocated_buffers.push_back(buf);
        }
        return buf;
    }

    std::vector<std::weak_ptr<mg::Buffer>>& allocated_buffers;
private:
    std::shared_ptr<mg::GraphicBufferAllocator> const underlying_allocator;
    std::mutex& buffer_mutex;
};

struct StubPlatform : public mg::Platform
{
    StubPlatform(std::shared_ptr<mir::Fd> const& last_fd)
        : last_fd(last_fd),
          underlying_platform{mtf::make_stubbed_server_graphics_platform({geom::Rectangle{{0,0},{1,1}}})}
    {
    }

    mir::UniqueModulePtr<mg::GraphicBufferAllocator> create_buffer_allocator(
        mg::Display const& output) override
    {
        return mir::make_module_ptr<RecordingBufferAllocator>(
            underlying_platform->create_buffer_allocator(output),
            allocated_buffers,
            buffer_mutex);
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

    std::vector<std::weak_ptr<mg::Buffer>> get_allocated_buffers()
    {
        std::lock_guard<std::mutex> lock{buffer_mutex};
        return allocated_buffers;
    }

    mg::NativeRenderingPlatform* native_rendering_platform() override { return nullptr; }
    mg::NativeDisplayPlatform* native_display_platform() override { return nullptr; }
    std::vector<mir::ExtensionDescription> extensions() const override { return {}; }

    std::shared_ptr<mir::Fd> const last_fd;
    std::shared_ptr<mg::Platform> const underlying_platform;

private:
    std::mutex buffer_mutex;
    std::vector<std::weak_ptr<mg::Buffer>> allocated_buffers;
};

struct ExchangeServerConfiguration : mtf::StubbedServerConfiguration
{
    ExchangeServerConfiguration(
        std::shared_ptr<mir::Fd> const& last_unpacked_fd) :
        platform{std::make_shared<StubPlatform>(last_unpacked_fd)}
    {
    }

    std::shared_ptr<mg::Platform> the_graphics_platform() override
    {
        return platform;
    }

    std::shared_ptr<StubPlatform> const platform;
};

struct SubmitBuffer : mir_test_framework::InProcessServer
{
    std::shared_ptr<mir::Fd> last_unpacked_fd{std::make_shared<mir::Fd>()};
    ExchangeServerConfiguration server_configuration{last_unpacked_fd};
    mir::DefaultServerConfiguration& server_config() override { return server_configuration; }

    std::vector<std::weak_ptr<mg::Buffer>> get_allocated_buffers()
    {
        return server_configuration.platform->get_allocated_buffers();
    }

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
        return cv.wait_for(lk, std::chrono::seconds(500), [this]() {return arrived;});
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
bool spin_wait_for_id(
    std::function<std::vector<mg::BufferID>()> const& id_generator,
    MirWindow* window,
    std::chrono::time_point<Clock> const& pt)
{
    while(Clock::now() < pt)
    {
        for (auto const& id : id_generator())
        {
            if (mir_debug_window_current_buffer_id(window) == id.as_value())
                return true;
        }
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

    mp::BufferAllocation allocation_request;
    auto allocate = allocation_request.add_buffer_requests();
    allocate->set_buffer_usage(static_cast<int32_t>(mg::BufferUsage::software));
    allocate->set_pixel_format(mir_pixel_format_abgr_8888);
    allocate->set_width(320);
    allocate->set_height(240);

    ASSERT_THAT(allocate_buffers(server, allocation_request), DidNotTimeOut());

    buffer_request.mutable_buffer()->set_buffer_id(
        get_allocated_buffers().back().lock()->id().as_value());
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

    for(int i = 0; i < 5; ++i)
    {
        mp::BufferAllocation allocation_request;
        auto allocate = allocation_request.add_buffer_requests();
        allocate->set_buffer_usage(static_cast<int32_t>(mg::BufferUsage::software));
        allocate->set_pixel_format(mir_pixel_format_abgr_8888);
        allocate->set_width(320);
        allocate->set_height(240);

        allocate_buffers(server, allocation_request);
    }

    mp::BufferRequest request;
    for (auto weak_buffer : get_allocated_buffers())
    {
        buffer_request.mutable_buffer()->set_buffer_id(weak_buffer.lock()->id().as_value());
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

    EXPECT_TRUE(
        spin_wait_for_id(
            [this]()
            {
                /* We expect to receive one of the implicitly allocated buffers
                 * We don't care which one we get.
                 *
                 * We need to repeatedly check the allocated buffers, because
                 * buffer allocation is asynchronous WRT window creation.
                 */
                std::vector<mg::BufferID> candidate_ids;
                for (auto const weak_buffer : get_allocated_buffers())
                {
                    auto const buffer = weak_buffer.lock();
                    if (buffer)
                    {
                        // If there are any expired buffers we don't care about them
                        candidate_ids.push_back(buffer->id());
                    }
                }
                return candidate_ids;
            },
            window,
            timeout))
        << "failed to see buffer";

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
