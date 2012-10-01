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

#ifndef MIR_TEST_STUB_SERVER_H_
#define MIR_TEST_STUB_SERVER_H_

#include "mir_protobuf.pb.h"
#include "mir/thread/all.h" 

namespace mir
{
namespace test
{

struct StubServer : mir::protobuf::DisplayServer
{
    static const int file_descriptors = 5;

    std::string app_name;
    std::string surface_name;
    int surface_count;
    std::mutex guard;
    std::condition_variable wait_condition;
    int file_descriptor[file_descriptors];

    StubServer() : surface_count(0)
    {
        for (auto i = file_descriptor; i != file_descriptor+file_descriptors; ++i)
            *i = 0;
    }

    StubServer(StubServer const &) = delete;
    void create_surface(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        response->mutable_id()->set_value(13); // TODO distinct numbers & tracking
        response->set_width(request->width());
        response->set_height(request->height());
        response->set_pixel_format(request->pixel_format());
        response->mutable_buffer();

        std::unique_lock<std::mutex> lock(guard);
        surface_name = request->surface_name();
        ++surface_count;
        wait_condition.notify_one();

        done->Run();
    }

    void next_buffer(
        ::google::protobuf::RpcController* /*controller*/,
        ::mir::protobuf::SurfaceId const* /*request*/,
        ::mir::protobuf::Buffer* /*response*/,
        ::google::protobuf::Closure* done)
    {
        std::unique_lock<std::mutex> lock(guard);
        wait_condition.notify_one();
        done->Run();
    }


    void release_surface(::google::protobuf::RpcController* /*controller*/,
                         const ::mir::protobuf::SurfaceId* /*request*/,
                         ::mir::protobuf::Void* /*response*/,
                         ::google::protobuf::Closure* /*done*/)
    {
        // TODO need some tests for releasing surfaces
    }


    void connect(
        ::google::protobuf::RpcController*,
                         const ::mir::protobuf::ConnectParameters* request,
                         ::mir::protobuf::Connection*,
                         ::google::protobuf::Closure* done)
    {
        app_name = request->application_name();
        done->Run();
    }

    void disconnect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::Void* /*request*/,
                 mir::protobuf::Void* /*response*/,
                 google::protobuf::Closure* done)
    {
        std::unique_lock<std::mutex> lock(guard);
        wait_condition.notify_one();
        done->Run();
    }

    void test_file_descriptors(::google::protobuf::RpcController* ,
                         const ::mir::protobuf::Void* ,
                         ::mir::protobuf::Buffer* fds,
                         ::google::protobuf::Closure* done)
    {
        for (int i = 0; i != file_descriptors; ++i)
        {
            static char const test_file_fmt[] = "fd_test_file%d";
            char test_file[sizeof test_file_fmt];
            sprintf(test_file, test_file_fmt, i);
            remove(test_file);
            file_descriptor[i] = open(test_file, O_CREAT, S_IWUSR|S_IRUSR);

            fds->add_fd(file_descriptor[i]);
        }

        done->Run();
    }

    void close_files()
    {
        for (auto i = file_descriptor; i != file_descriptor+file_descriptors; ++i)
            close(*i), *i = 0;
    }
};

const int StubServer::file_descriptors;
}
}
#endif /* MIR_TEST_STUB_SERVER_H_ */
