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
#include "mir/test/doubles/stub_client_buffer_factory.h"
#include "mir/test/doubles/mock_client_buffer.h"
#include "mir/test/fake_shared.h"
#include "src/client/presentation_chain.h"
#include "src/client/buffer_factory.h"
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

namespace
{
struct PresentationChain : Test
{
    PresentationChain()
    {
        ipc_buf.set_width(size.width.as_int());
        ipc_buf.set_height(size.height.as_int());
        ipc_buf.set_buffer_id(buffer_id);
    }

    int rpc_id { 33 };
    MirConnection* connection {reinterpret_cast<MirConnection*>(this)};
    geom::Size size {100, 200};
    MirPixelFormat format = mir_pixel_format_abgr_8888;
    MirBufferUsage usage = mir_buffer_usage_software;
    mtd::MockProtobufServer mock_server;
    int buffer_id {4312};
    mp::Buffer ipc_buf;
};

MATCHER_P(BufferRequestMatches, val, "")
{
    return ((arg->id().value() == val.id().value()) &&
        arg->has_buffer() &&
        val.has_buffer() &&
        arg->buffer().buffer_id() == val.buffer().buffer_id());
}

void buffer_callback(MirPresentationChain*, MirBuffer*, void*)
{
}
}

TEST_F(PresentationChain, returns_associated_connection)
{
    mcl::PresentationChain chain(
        connection, rpc_id, mock_server,
        std::make_shared<mtd::StubClientBufferFactory>(),
        std::make_shared<mcl::BufferFactory>());
    EXPECT_THAT(chain.connection(), Eq(connection));
}

TEST_F(PresentationChain, returns_associated_rpc_id)
{
    mcl::PresentationChain chain(
        connection, rpc_id, mock_server,
        std::make_shared<mtd::StubClientBufferFactory>(),
        std::make_shared<mcl::BufferFactory>());
    EXPECT_THAT(chain.rpc_id(), Eq(rpc_id));
}

TEST_F(PresentationChain, submits_buffer_when_asked)
{
    mp::BufferRequest request;
    request.mutable_id()->set_value(rpc_id);
    request.mutable_buffer()->set_buffer_id(buffer_id);

    EXPECT_CALL(mock_server, submit_buffer(BufferRequestMatches(request),_,_))
        .WillOnce(mtd::RunProtobufClosure());

    mcl::Buffer buffer(buffer_callback, nullptr, buffer_id, nullptr, nullptr);
    mcl::PresentationChain chain(
        connection, rpc_id, mock_server,
        std::make_shared<mtd::StubClientBufferFactory>(),
        std::make_shared<mcl::BufferFactory>());
    chain.submit_buffer(reinterpret_cast<MirBuffer*>(&buffer));
} 

TEST_F(PresentationChain, double_submission_throws)
{
    EXPECT_CALL(mock_server, submit_buffer(_,_,_))
        .WillOnce(mtd::RunProtobufClosure());

    mcl::Buffer buffer(buffer_callback, nullptr, buffer_id, nullptr, nullptr);
    mcl::PresentationChain chain(
        connection, rpc_id, mock_server,
        std::make_shared<mtd::StubClientBufferFactory>(),
        std::make_shared<mcl::BufferFactory>());

    chain.submit_buffer(reinterpret_cast<MirBuffer*>(&buffer));
    EXPECT_THROW({
        chain.submit_buffer(reinterpret_cast<MirBuffer*>(&buffer));
    }, std::logic_error);
}
