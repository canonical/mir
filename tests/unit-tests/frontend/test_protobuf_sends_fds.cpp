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

#include "mir/frontend/connector.h"
#include "src/server/frontend/resource_cache.h"

#include "mir_protobuf.pb.h"

#include "mir_test_doubles/stub_ipc_factory.h"
#include "mir_test/stub_server_tool.h"
#include "mir_test/test_protobuf_client.h"
#include "mir_test/test_protobuf_server.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fcntl.h>

#include <stdexcept>
#include <memory>
#include <string>

namespace mf = mir::frontend;
namespace mt = mir::test;

namespace mir
{
namespace test
{
struct StubServerFd : public StubServerTool
{
    static const int file_descriptors = 5;
    int file_descriptor[file_descriptors];

    StubServerFd()
    {
        for (auto i = file_descriptor; i != file_descriptor+file_descriptors; ++i)
            *i = 0;
    }

    void test_file_descriptors(::google::protobuf::RpcController* ,
                         const ::mir::protobuf::Void* ,
                         ::mir::protobuf::Buffer* buffer,
                         ::google::protobuf::Closure* done)
    {
        for (int i = 0; i != file_descriptors; ++i)
        {

            static char const test_file_fmt[] = "fd_test_file%d";
            char test_file[sizeof test_file_fmt];
            sprintf(test_file, test_file_fmt, i);
            remove(test_file);
            file_descriptor[i] = open(test_file, O_CREAT, S_IWUSR|S_IRUSR);

            buffer->add_fd(file_descriptor[i]);
        }

        done->Run();
    }

    void close_files()
    {
        for (auto i = file_descriptor; i != file_descriptor+file_descriptors; ++i)
            close(*i), *i = 0;
    }

};
const int StubServerFd::file_descriptors;
}

struct ProtobufSocketCommunicatorFD : public ::testing::Test
{
    void SetUp()
    {
        stub_server_tool = std::make_shared<mt::StubServerFd>();
        stub_server = std::make_shared<mt::TestProtobufServer>("./test_socket", stub_server_tool);
        stub_server->comm->start();

        stub_client = std::make_shared<mt::TestProtobufClient>("./test_socket", 500);
        stub_client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);
    }

    void TearDown()
    {
        stub_server.reset();
    }

    std::shared_ptr<mt::TestProtobufClient> stub_client;
    std::shared_ptr<mt::StubServerFd> stub_server_tool;

private:
    std::shared_ptr<mt::TestProtobufServer> stub_server;
};

TEST_F(ProtobufSocketCommunicatorFD, test_file_descriptors)
{
    mir::protobuf::Buffer fds;

    stub_client->display_server.test_file_descriptors(0, &stub_client->ignored, &fds,
        google::protobuf::NewCallback(stub_client.get(), &mt::TestProtobufClient::tfd_done));

    stub_client->wait_for_tfd_done();

    ASSERT_EQ(stub_server_tool->file_descriptors, fds.fd_size());

    for (int i  = 0; i != stub_server_tool->file_descriptors; ++i)
    {
        int const fd = fds.fd(i);
        EXPECT_NE(-1, fd);

        for (int j  = 0; j != stub_server_tool->file_descriptors; ++j)
        {
            EXPECT_NE(stub_server_tool->file_descriptor[j], fd);
        }
    }

    stub_server_tool->close_files();
    for (int i  = 0; i != stub_server_tool->file_descriptors; ++i)
        close(fds.fd(i));
}
}
