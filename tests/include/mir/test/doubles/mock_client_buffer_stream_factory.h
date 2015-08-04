/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_CLIENT_BUFFER_STREAM_FACTORY_H_
#define MIR_TEST_DOUBLES_MOCK_CLIENT_BUFFER_STREAM_FACTORY_H_

#include "src/client/client_buffer_stream_factory.h"
#include "src/client/buffer_stream.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockClientBufferStreamFactory : public client::ClientBufferStreamFactory
{
    MOCK_METHOD4(make_consumer_stream, std::shared_ptr<client::ClientBufferStream>(
        MirConnection*, client::rpc::DisplayServer&,
        protobuf::BufferStream const&, std::string const&));
    MOCK_METHOD4(make_producer_stream, std::shared_ptr<client::ClientBufferStream>(
        MirConnection*, client::rpc::DisplayServer&,
        protobuf::BufferStream const&, std::string const&));
    MOCK_METHOD5(make_producer_stream, client::ClientBufferStream*(
        MirConnection*, client::rpc::DisplayServer&,
        protobuf::BufferStreamParameters const&, mir_buffer_stream_callback callback, void* context));
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_CLIENT_BUFFER_STREAM_FACTORY_H_
