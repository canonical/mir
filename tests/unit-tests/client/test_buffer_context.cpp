/*
 * Copyright © 2016 Canonical Ltd.
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

#include "mir/test/doubles/mock_protobuf_server.h"
#include "src/client/buffer_context.h"
#include "mir/client_buffer_factory.h"

#include <mutex>
#include <condition_variable>
#include <gtest/gtest.h>
using namespace testing;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;
namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;

struct MockClientBufferFactory : public mcl::ClientBufferFactory
{
    MockClientBufferFactory()
    {
/*        ON_CALL(*this, create_buffer(_,_,_))
            .WillByDefault(Invoke([](
                    std::shared_ptr<MirBufferPackage> const&, geom::Size size, MirPixelFormat)
                {
                    return std::make_shared<NiceMock<MockBuffer>>(size);
                }));
*/
    }
    MOCK_METHOD3(create_buffer, std::shared_ptr<mcl::ClientBuffer>(
        std::shared_ptr<MirBufferPackage> const&, geom::Size, MirPixelFormat));
};

struct BufferContext : Test
{
    int rpc_id = 33;
    MirConnection* connection {reinterpret_cast<MirConnection*>(this)};
    std::shared_ptr<MirWaitHandle> wait_handle;
    geom::Size size {100, 200};
    MirPixelFormat format = mir_pixel_format_abgr_8888;
    MirBufferUsage usage = mir_buffer_usage_software;
    mtd::MockProtobufServer mock_server;
    std::shared_ptr<MockClientBufferFactory> const factory {
        std::make_shared<NiceMock<MockClientBufferFactory>>() };
};

struct BContext
{
    std::mutex mut;
    std::condition_variable cv;
    MirBuffer* buffer = nullptr;
};

static void buffer_callback(MirBufferContext*, MirBuffer* buffer, void* context)
{
    auto c = static_cast<BContext*>(context);
    std::unique_lock<std::mutex> lk(c->mut);
    c->buffer = buffer;
    c->cv.notify_all();
}

struct BufferCount
{
    std::mutex mut;
    std::condition_variable cv;
    MirBuffer* buffer = nullptr;
    unsigned int count = 0;
};

static void counting_buffer_callback(MirBufferContext*, MirBuffer* buffer, void* context)
{
    BufferCount* c = static_cast<BufferCount*>(context);
    c->buffer = buffer;
    c->count = c->count + 1;
}

MATCHER_P(BufferRequestMatches, val, "")
{
#if 0
    return ((arg->id().value() == val.id().value()) &&
        arg->has_buffer() &&
        val.has_buffer() &&
        arg->buffer().buffer_id() == val.buffer().buffer_id());
#else
    (void)arg; (void)val;
    return true;
#endif
}

MATCHER_P(BufferAllocationMatches, val, "")
{
    return ((arg->id().value() == val.id().value()) &&
        (arg->buffer_requests_size() == 1) &&
        (val.buffer_requests_size() == 1) &&
        (arg->buffer_requests(0).width() == val.buffer_requests(0).width()) &&
        (arg->buffer_requests(0).height() == val.buffer_requests(0).height()) &&
        (arg->buffer_requests(0).pixel_format() == val.buffer_requests(0).pixel_format()) &&
        (arg->buffer_requests(0).buffer_usage() == val.buffer_requests(0).buffer_usage()));
}

MATCHER_P(BufferReleaseMatches, val, "")
{
    return ((arg->id().value() == val.id().value()) &&
        (arg->buffers_size() == 1) &&
        (val.buffers_size() == 1) &&
        (arg->buffers(0).buffer_id() == val.buffers(0).buffer_id()));
}

TEST_F(BufferContext, returns_associated_connection)
{
    mcl::BufferContext context(connection, wait_handle, rpc_id, mock_server, factory);
    EXPECT_THAT(context.connection(), Eq(connection));
}

TEST_F(BufferContext, returns_associated_rpc_id)
{
    mcl::BufferContext context(connection, wait_handle, rpc_id, mock_server, factory);
    EXPECT_THAT(context.rpc_id(), Eq(rpc_id));
}

TEST_F(BufferContext, creates_buffer_when_asked)
{
    BContext buffer;
    mp::BufferAllocation mp_alloc;
    auto params = mp_alloc.add_buffer_requests();
    params->set_width(size.width.as_int());
    params->set_height(size.height.as_int());
    params->set_buffer_usage(usage);
    params->set_pixel_format(format);
    mp_alloc.mutable_id()->set_value(rpc_id);

    EXPECT_CALL(mock_server, allocate_buffers(BufferAllocationMatches(mp_alloc),_,_))
        .WillOnce(mtd::RunProtobufClosure());
    EXPECT_CALL(*factory, create_buffer(_, size, format));
 
    mcl::BufferContext context(connection, wait_handle, rpc_id, mock_server, factory);
    context.allocate_buffer(size, format, usage, buffer_callback, &buffer);

    std::unique_lock<std::mutex> lk(buffer.mut);
    EXPECT_THAT(buffer.buffer, Eq(nullptr));
    lk.unlock();

    mp::Buffer ipc_buf;
    ipc_buf.set_width(size.width.as_int());
    ipc_buf.set_height(size.height.as_int());
    context.buffer_available(ipc_buf);

    lk.lock();
    EXPECT_TRUE(buffer.cv.wait_for(lk, std::chrono::seconds(5), [&] { return buffer.buffer; }));
}

TEST_F(BufferContext, creates_correct_buffer_when_buffers_arrive)
{
    size_t const num_buffers = 3u;
    std::array<geom::Size, num_buffers> sizes {
        geom::Size{2,2},
        geom::Size{2,1},
        geom::Size{1,2}
    };

    std::array<mp::Buffer, num_buffers> ipc_buf;
    std::array<BContext, num_buffers> buffer;
    std::array<mp::BufferAllocation, num_buffers> mp_alloc;
    Sequence seq;
    for (auto i = 0u; i < num_buffers; i++)
    {
        mp_alloc[i].mutable_id()->set_value(rpc_id);
        auto params = mp_alloc[i].add_buffer_requests();
        params->set_width(sizes[i].width.as_int());
        params->set_height(sizes[i].height.as_int());
        params->set_buffer_usage(usage);
        params->set_pixel_format(format);

        EXPECT_CALL(mock_server, allocate_buffers(BufferAllocationMatches(mp_alloc[i]),_,_))
            .InSequence(seq)
            .WillOnce(mtd::RunProtobufClosure());

        ipc_buf[i].set_buffer_id(i);
        ipc_buf[i].set_width(sizes[i].width.as_int());
        ipc_buf[i].set_height(sizes[i].height.as_int());
    }

    mcl::BufferContext context(connection, wait_handle, rpc_id, mock_server, factory);

    for (auto i = 0u; i < num_buffers; i++)
        context.allocate_buffer(sizes[i], format, usage, buffer_callback, &buffer[i]);

    context.buffer_available(ipc_buf[1]);
    EXPECT_THAT(buffer[0].buffer, Eq(nullptr));
    EXPECT_THAT(buffer[1].buffer, Ne(nullptr));
    EXPECT_THAT(buffer[2].buffer, Eq(nullptr));

    context.buffer_available(ipc_buf[2]);
    EXPECT_THAT(buffer[0].buffer, Eq(nullptr));
    EXPECT_THAT(buffer[2].buffer, Ne(nullptr));

    context.buffer_available(ipc_buf[0]);
    EXPECT_THAT(buffer[0].buffer, Ne(nullptr));
}

TEST_F(BufferContext, frees_buffer_when_asked)
{
    BContext buffer;
    int buffer_id = 4312;
    mp::Buffer ipc_buf;
    ipc_buf.set_width(size.width.as_int());
    ipc_buf.set_height(size.height.as_int());
    ipc_buf.set_buffer_id(buffer_id);
    gp::Closure* closure = nullptr;
    mp::BufferRelease release_msg;
    release_msg.mutable_id()->set_value(rpc_id);
    auto released_buffer = release_msg.add_buffers();
    released_buffer->set_buffer_id(buffer_id);

    EXPECT_CALL(mock_server, allocate_buffers(_,_,_))
        .WillOnce(mtd::RunProtobufClosure());
    EXPECT_CALL(mock_server, release_buffers(BufferReleaseMatches(release_msg),_,_))
        .WillOnce(mtd::RunProtobufClosure());

    mcl::BufferContext context(connection, wait_handle, rpc_id, mock_server, factory);
    context.allocate_buffer(size, format, usage, buffer_callback, &buffer);
    context.buffer_available(ipc_buf);
    std::unique_lock<std::mutex> lk(buffer.mut);
    EXPECT_TRUE(buffer.cv.wait_for(lk, std::chrono::seconds(5), [&] { return buffer.buffer; }));

    ASSERT_THAT(buffer.buffer, Ne(nullptr));
    context.release_buffer(buffer.buffer);

} 

TEST_F(BufferContext, submits_buffer_when_asked)
{
    BContext buffer;
    gp::Closure* closure = nullptr;
    int buffer_id = 4312;

    mp::Buffer ipc_buf;
    ipc_buf.set_width(size.width.as_int());
    ipc_buf.set_height(size.height.as_int());
    ipc_buf.set_buffer_id(buffer_id);
    mp::BufferRequest request;
    request.mutable_id()->set_value(rpc_id);
    request.mutable_buffer()->set_buffer_id(buffer_id);

    EXPECT_CALL(mock_server, allocate_buffers(_,_,_))
        .WillOnce(mtd::RunProtobufClosure());
    EXPECT_CALL(mock_server, submit_buffer(BufferRequestMatches(request),_,_))
        .WillOnce(mtd::RunProtobufClosure());

    mcl::BufferContext context(connection, wait_handle, rpc_id, mock_server, factory);
    context.allocate_buffer(size, format, usage, buffer_callback, &buffer);
    context.buffer_available(ipc_buf);
    std::unique_lock<std::mutex> lk(buffer.mut);
    EXPECT_TRUE(buffer.cv.wait_for(lk, std::chrono::seconds(5), [&] { return buffer.buffer; }));

    context.submit_buffer(buffer.buffer);
} 

TEST_F(BufferContext, double_submission_throws)
{
    BContext buffer;
    gp::Closure* closure = nullptr;
    int buffer_id = 4312;
    mp::Buffer ipc_buf;
    ipc_buf.set_width(size.width.as_int());
    ipc_buf.set_height(size.height.as_int());
    ipc_buf.set_buffer_id(buffer_id);

    EXPECT_CALL(mock_server, allocate_buffers(_,_,_))
        .WillOnce(mtd::RunProtobufClosure());
    EXPECT_CALL(mock_server, submit_buffer(_,_,_))
        .WillOnce(mtd::RunProtobufClosure());

    mcl::BufferContext context(connection, wait_handle, rpc_id, mock_server, factory);
    context.allocate_buffer(size, format, usage, buffer_callback, &buffer);
    context.buffer_available(ipc_buf);
    std::unique_lock<std::mutex> lk(buffer.mut);
    EXPECT_TRUE(buffer.cv.wait_for(lk, std::chrono::seconds(5), [&] { return buffer.buffer; }));

    context.submit_buffer(buffer.buffer);
    EXPECT_THROW({
        context.submit_buffer(buffer.buffer);
    }, std::logic_error);
}

TEST_F(BufferContext, callback_invoked_when_buffer_returned_from_allocation_and_submission)
{
    BufferCount counter;
    gp::Closure* closure = nullptr;
    int buffer_id = 4312;
    mp::Buffer ipc_buf;
    ipc_buf.set_width(size.width.as_int());
    ipc_buf.set_height(size.height.as_int());
    ipc_buf.set_buffer_id(buffer_id);

    EXPECT_CALL(mock_server, allocate_buffers(_,_,_))
        .WillOnce(mtd::RunProtobufClosure());
    EXPECT_CALL(mock_server, submit_buffer(_,_,_))
        .WillOnce(mtd::RunProtobufClosure());

    mcl::BufferContext context(connection, wait_handle, rpc_id, mock_server, factory);
    context.allocate_buffer(size, format, usage, counting_buffer_callback, &counter);
    context.buffer_available(ipc_buf);
    std::unique_lock<std::mutex> lk(counter.mut);
    EXPECT_TRUE(counter.cv.wait_for(lk, std::chrono::seconds(5), [&] { return counter.buffer; }));
    lk.unlock();

    context.submit_buffer(counter.buffer);
    context.buffer_available(ipc_buf);

    lk.lock();
    EXPECT_THAT(counter.count, Eq(2));
}
