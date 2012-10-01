/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_STUB_SERVER_ERROR_H_
#define MIR_TEST_STUB_SERVER_ERROR_H_

#include "mir_protobuf.pb.h"
#include "mir/thread/all.h" 

namespace mir
{
namespace test
{
struct ErrorServer : mir::protobuf::DisplayServer
{
    static std::string const test_exception_text;

    void create_surface(
        google::protobuf::RpcController*,
        const protobuf::SurfaceParameters*,
        protobuf::Surface*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }

    void release_surface(
        google::protobuf::RpcController*,
        const protobuf::SurfaceId*,
        protobuf::Void*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }


    void connect(
        ::google::protobuf::RpcController*,
        const ::mir::protobuf::ConnectParameters*,
        ::mir::protobuf::Connection*,
        ::google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }

    void disconnect(
        google::protobuf::RpcController*,
        const protobuf::Void*,
        protobuf::Void*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }

    void test_file_descriptors(
        google::protobuf::RpcController*,
        const protobuf::Void*,
        protobuf::Buffer*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }
};

std::string const ErrorServer::test_exception_text{"test exception text"};
}
}
#endif /* MIR_TEST_STUB_SERVER_ERROR_H_ */
