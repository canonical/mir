/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_PROTOBUF_SERVER_H_
#define MIR_TEST_DOUBLES_MOCK_PROTOBUF_SERVER_H_

#include "src/client/rpc/mir_display_server.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
ACTION(RunProtobufClosure)
{
    arg2->Run();
}

struct MockProtobufServer : public client::rpc::DisplayServer
{
    MockProtobufServer() : client::rpc::DisplayServer(nullptr)
    {
        using namespace testing;
        ON_CALL(*this, configure_buffer_stream(_,_,_))
            .WillByDefault(RunProtobufClosure());
        ON_CALL(*this, submit_buffer(_,_,_))
            .WillByDefault(RunProtobufClosure());
        ON_CALL(*this, allocate_buffers(_,_,_))
            .WillByDefault(DoAll(InvokeWithoutArgs([this]{ alloc_count++; }), RunProtobufClosure()));
        ON_CALL(*this, release_buffers(_,_,_))
            .WillByDefault(RunProtobufClosure());
    }

    MOCK_METHOD3(configure_buffer_stream, void(
        mir::protobuf::StreamConfiguration const*, 
        protobuf::Void*,
        google::protobuf::Closure*));
    MOCK_METHOD3(screencast_buffer, void(
        protobuf::ScreencastId const*,
        protobuf::Buffer*,
        google::protobuf::Closure*));
    MOCK_METHOD3(allocate_buffers, void(
        mir::protobuf::BufferAllocation const*,
        mir::protobuf::Void*,
        google::protobuf::Closure*));
    MOCK_METHOD3(release_buffers, void(
        mir::protobuf::BufferRelease const*,
        mir::protobuf::Void*,
        google::protobuf::Closure*));
    MOCK_METHOD3(submit_buffer, void(
        protobuf::BufferRequest const*,
        protobuf::Void*,
        google::protobuf::Closure*));
    MOCK_METHOD3(exchange_buffer, void(
        protobuf::BufferRequest const*,
        protobuf::Buffer*,
        google::protobuf::Closure*));
    MOCK_METHOD3(create_buffer_stream, void(
        protobuf::BufferStreamParameters const*,
        protobuf::BufferStream*,
        google::protobuf::Closure*));
    unsigned int alloc_count{0};
};
}
}
}
#endif
