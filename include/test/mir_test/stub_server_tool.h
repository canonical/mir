/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_STUB_SERVER_TOOL_H_
#define MIR_TEST_STUB_SERVER_TOOL_H_

#include "mir_protobuf.pb.h"
#include <condition_variable>
#include <mutex>

namespace mir
{
namespace test
{

struct StubServerTool : mir::protobuf::DisplayServer
{
    StubServerTool()
        : drm_magic{0}
    {
    }

    virtual void create_surface(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done) override
    {
        response->mutable_id()->set_value(13); // TODO distinct numbers & tracking
        response->set_width(request->width());
        response->set_height(request->height());
        response->set_pixel_format(request->pixel_format());
        response->mutable_buffer()->set_buffer_id(22);

        std::unique_lock<std::mutex> lock(guard);
        surface_name = request->surface_name();
        wait_condition.notify_one();

        done->Run();
    }

    virtual void next_buffer(
        ::google::protobuf::RpcController* /*controller*/,
        ::mir::protobuf::SurfaceId const* /*request*/,
        ::mir::protobuf::Buffer* response,
        ::google::protobuf::Closure* done) override
    {
        response->set_buffer_id(22);

        std::unique_lock<std::mutex> lock(guard);
        wait_condition.notify_one();
        done->Run();
    }


    virtual void release_surface(::google::protobuf::RpcController* /*controller*/,
                         const ::mir::protobuf::SurfaceId* /*request*/,
                         ::mir::protobuf::Void* /*response*/,
                         ::google::protobuf::Closure* done) override
    {
        done->Run();
    }


    virtual void connect(
        ::google::protobuf::RpcController*,
                         const ::mir::protobuf::ConnectParameters* request,
                         ::mir::protobuf::Connection* connect_msg,
                         ::google::protobuf::Closure* done) override
    {
        app_name = request->application_name();
        connect_msg->set_error("");
        done->Run();
    }

    virtual void disconnect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::Void* /*request*/,
                 mir::protobuf::Void* /*response*/,
                 google::protobuf::Closure* done) override
    {
        std::unique_lock<std::mutex> lock(guard);
        wait_condition.notify_one();
        done->Run();
    }

    virtual void drm_auth_magic(google::protobuf::RpcController* /*controller*/,
                                const mir::protobuf::DRMMagic* request,
                                mir::protobuf::DRMAuthMagicStatus* response,
                                google::protobuf::Closure* done) override
    {
        std::unique_lock<std::mutex> lock(guard);
        drm_magic = request->magic();
        response->set_status_code(0);
        wait_condition.notify_one();
        done->Run();
    }

    virtual void configure_display(::google::protobuf::RpcController*,
                       const ::mir::protobuf::DisplayConfiguration*,
                       ::mir::protobuf::DisplayConfiguration*,
                       ::google::protobuf::Closure* done) override
    {
        done->Run();
    }

    std::mutex guard;
    std::string surface_name;
    std::condition_variable wait_condition;
    std::string app_name;
    unsigned int drm_magic;
};

}
}
#endif /* MIR_TEST_STUB_SERVER_TOOL_H_ */
