/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "src/server/frontend/display_server.h"
#include <mir/test/doubles/stub_display_server.h>

#include <condition_variable>
#include <mutex>

namespace mir
{
namespace test
{

struct StubServerTool : doubles::StubDisplayServer
{
    virtual void create_surface(
        mir::protobuf::SurfaceParameters const* request,
        mir::protobuf::Surface* response,
        google::protobuf::Closure* done) override
    {
        response->mutable_id()->set_value(13); // TODO distinct numbers & tracking
        response->set_width(request->width());
        response->set_height(request->height());
        response->set_pixel_format(request->pixel_format());
        response->mutable_buffer()->set_buffer_id(22);

        std::lock_guard<std::mutex> lock(guard);
        surf_name = request->surface_name();
        signal.notify_one();

        done->Run();
    }

    virtual void submit_buffer(
        mir::protobuf::BufferRequest const* /*request*/,
        mir::protobuf::Void*,
        google::protobuf::Closure* done) override
    {
        done->Run();
    }


    virtual void release_surface(
        mir::protobuf::SurfaceId const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* done) override
    {
        done->Run();
    }


    virtual void connect(
        mir::protobuf::ConnectParameters const* request,
        mir::protobuf::Connection* connect_msg,
        google::protobuf::Closure* done) override
    {
        {
            std::lock_guard<std::mutex> lock(guard);
            app_name = request->application_name();
        }
        // If you check out MirConnection::connected either the platform and display_configuration
        // have to be set or the error has to be set, otherwise we die and fail to callback.
        //
        // Since setting the error to "" means that mir_connection_is_valid will return false
        // with a confusing non-error-message, instead clear the error and set the platform et al.
        connect_msg->clear_error();
        connect_msg->set_allocated_platform(new mir::protobuf::Platform{});
        connect_msg->set_allocated_display_configuration(new mir::protobuf::DisplayConfiguration{});
        done->Run();
    }

    virtual void disconnect(
        mir::protobuf::Void const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* done) override
    {
        std::lock_guard<std::mutex> lock(guard);
        //FIXME: huh? What's the condition here?
        signal.notify_one();
        done->Run();
    }

    virtual void configure_display(
        mir::protobuf::DisplayConfiguration const*,
        mir::protobuf::DisplayConfiguration*,
        google::protobuf::Closure* done) override
    {
        done->Run();
    }

    virtual void apply_input_configuration(
        mir::protobuf::InputConfigurationRequest const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* done) override
    {
        done->Run();
    }

    virtual void set_base_input_configuration(
        mir::protobuf::InputConfigurationRequest const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* done) override
    {
        done->Run();
    }
    virtual void allocate_buffers(
        mir::protobuf::BufferAllocation const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* done) override
    {
        done->Run();
    }
    void release_buffers(
        mir::protobuf::BufferRelease const* /*request*/,
        mir::protobuf::Void* /*response*/,
        google::protobuf::Closure* done) override
    {
        done->Run();
    }

    std::string application_name() const
    {
        std::lock_guard<std::mutex> lock(guard);
        return app_name;
    }

    std::string surface_name() const
    {
        std::lock_guard<std::mutex> lock(guard);
        return surf_name;
    }

    std::mutex mutable guard;
    std::string surf_name;
    std::condition_variable signal;
    std::string app_name;
};

}
}
#endif /* MIR_TEST_STUB_SERVER_TOOL_H_ */
