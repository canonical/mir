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

#include "mir/frontend/protobuf_asio_communicator.h"
#include "mir/frontend/resource_cache.h"

#include "mir_protobuf.pb.h"
#include "mir_rpc_channel.h"

#include "mir_test/mock_ipc_factory.h"
#include "mir_test/mock_logger.h"
#include "mir_test/mock_server_tool.h"
#include "mir_test/test_client.h"
#include "mir_test/test_server.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <memory>
#include <string>

namespace mf = mir::frontend;
namespace mt = mir::test;

namespace mir
{
namespace test
{
struct MockServerFd : public MockServerTool
{
    static const int file_descriptors = 5;
    int file_descriptor[file_descriptors];

    MockServerFd()
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
const int MockServerFd::file_descriptors;
}

struct ProtobufAsioCommunicatorFD : public ::testing::Test
{
    void SetUp()
    {
        mock_server_tool = std::make_shared<mt::MockServerFd>();
        mock_server = std::make_shared<mt::TestServer>("./test_socket", mock_server_tool);
 
        ::testing::Mock::VerifyAndClearExpectations(mock_server->factory.get());
        EXPECT_CALL(*mock_server->factory, make_ipc_server()).Times(1);

        mock_server->comm.start();

        mock_client = std::make_shared<mt::TestClient>("./test_socket");
        mock_client->connect_parameters.set_application_name(__PRETTY_FUNCTION__);
    }

    void TearDown()
    {
        mock_server->comm.stop();
    }

    std::shared_ptr<mt::TestClient> mock_client;
    std::shared_ptr<mt::MockServerFd> mock_server_tool;

private:
    std::shared_ptr<mt::TestServer> mock_server;
};

TEST_F(ProtobufAsioCommunicatorFD, test_file_descriptors)
{
    mir::protobuf::Buffer fds;

    mock_client->display_server.test_file_descriptors(0, &mock_client->ignored, &fds,
        google::protobuf::NewCallback(mock_client.get(), &mt::TestClient::tfd_done));
    mock_client->wait_for_tfd_done();

    ASSERT_EQ(mock_server_tool->file_descriptors, fds.fd_size());

    for (int i  = 0; i != mock_server_tool->file_descriptors; ++i)
    {
        int const fd = fds.fd(i);
        EXPECT_NE(-1, fd);

        for (int j  = 0; j != mock_server_tool->file_descriptors; ++j)
        {
            EXPECT_NE(mock_server_tool->file_descriptor[j], fd);
        }
    }

    mock_server_tool->close_files();
    for (int i  = 0; i != mock_server_tool->file_descriptors; ++i)
        close(fds.fd(i));
}
}
