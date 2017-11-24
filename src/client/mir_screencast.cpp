/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_screencast.h"
#include "mir_connection.h"
#include "mir_protobuf.pb.h"
#include "make_protobuf_object.h"
#include "mir/mir_buffer_stream.h"
#include "mir/mir_buffer.h"
#include "mir/frontend/client_constants.h"
#include "mir_toolkit/mir_native_buffer.h"

#include <boost/throw_exception.hpp>
#include <algorithm>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace geom = mir::geometry;

namespace
{
mir::protobuf::ScreencastParameters serialize_spec(MirScreencastSpec const& spec)
{
    mp::ScreencastParameters message;

#define SERIALIZE_OPTION_IF_SET(option) \
    if (spec.option.is_set()) \
        message.set_##option(spec.option.value());

    SERIALIZE_OPTION_IF_SET(width);
    SERIALIZE_OPTION_IF_SET(height);
    SERIALIZE_OPTION_IF_SET(pixel_format);
    SERIALIZE_OPTION_IF_SET(mirror_mode);
    SERIALIZE_OPTION_IF_SET(num_buffers);

    if (spec.capture_region.is_set())
    {
        auto const region = spec.capture_region.value();
        message.mutable_region()->set_left(region.left);
        message.mutable_region()->set_top(region.top);
        message.mutable_region()->set_width(region.width);
        message.mutable_region()->set_height(region.height);
    }

    if (spec.num_buffers.is_set() && spec.num_buffers.value() == 0)
    {
        if (!spec.pixel_format.is_set())
            message.set_pixel_format(mir_pixel_format_abgr_8888);
        if (!spec.width.is_set())
            message.set_width(1);
        if (!spec.width.is_set())
            message.set_height(1);
    }
        
    return message;
}

//TODO: it should be up to the server to decide if the parameters are acceptable
void throw_if_invalid(MirScreencastSpec const& spec)
{
#define THROW_IF_UNSET(option) \
    if (!spec.option.is_set()) \
        BOOST_THROW_EXCEPTION(std::runtime_error("Unset "#option));

#define THROW_IF_EQ(option, val) \
    THROW_IF_UNSET(option); \
    if (spec.option.is_set() && spec.option.value() == val) \
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid "#option));

#define THROW_IF_ZERO(option) THROW_IF_EQ(option, 0)

    THROW_IF_UNSET(capture_region);

    if (spec.capture_region.is_set())
    {
        auto const region = spec.capture_region.value();
        if (region.width == 0)
            BOOST_THROW_EXCEPTION(std::runtime_error("Invalid capture region width"));
        if (region.height == 0)
            BOOST_THROW_EXCEPTION(std::runtime_error("Invalid capture region height"));
    }

    //zero nbuffers don't need to set pixel format, width, or height
    if (!spec.num_buffers.is_set() || spec.num_buffers.value() > 0)
    {
        THROW_IF_ZERO(width);
        THROW_IF_ZERO(height);
        THROW_IF_EQ(pixel_format, mir_pixel_format_invalid);
    }
}
}

MirScreencastSpec::MirScreencastSpec() = default;

MirScreencastSpec::MirScreencastSpec(MirConnection* connection)
    : connection{connection}
{
}

MirScreencastSpec::MirScreencastSpec(MirConnection* connection, MirScreencastParameters const& params)
    : connection{connection},
      width{params.width},
      height{params.height},
      pixel_format{params.pixel_format},
      capture_region{params.region}
{
}

MirScreencast::MirScreencast(std::string const& error)
    : protobuf_screencast{mcl::make_protobuf_object<mir::protobuf::Screencast>()}
{
    protobuf_screencast->set_error(error);
}

MirScreencast::MirScreencast(
    MirScreencastSpec const& spec,
    mir::client::rpc::DisplayServer& the_server,
    MirScreencastCallback callback, void* context)
    : server{&the_server},
      connection{spec.connection},
      protobuf_screencast{mcl::make_protobuf_object<mir::protobuf::Screencast>()},
      protobuf_void{mcl::make_protobuf_object<mir::protobuf::Void>()}
{
    throw_if_invalid(spec);
    auto const message = serialize_spec(spec);

    create_screencast_wait_handle.expect_result();
    server->create_screencast(
        &message,
        protobuf_screencast.get(),
        google::protobuf::NewCallback(
            this, &MirScreencast::screencast_created,
            callback, context));
}

MirWaitHandle* MirScreencast::creation_wait_handle()
{
    return &create_screencast_wait_handle;
}

bool MirScreencast::valid()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return !protobuf_screencast->has_error();
}

char const* MirScreencast::get_error_message()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (protobuf_screencast->has_error())
    {
        return protobuf_screencast->error().c_str();
    }
    return empty_error_message.c_str();
}

MirWaitHandle* MirScreencast::release(MirScreencastCallback callback, void* context)
{
    release_wait_handle.expect_result();
    if (valid() && server)
    {
        mp::ScreencastId screencast_id;
        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            screencast_id.set_value(protobuf_screencast->screencast_id().value());
        }
        server->release_screencast(
            &screencast_id,
            protobuf_void.get(),
            google::protobuf::NewCallback(
                this, &MirScreencast::released, callback, context));
    }
    else
    {
        callback(this, context);
        release_wait_handle.result_received();
    }

    return &release_wait_handle;
}

void MirScreencast::screencast_created(
    MirScreencastCallback callback, void* context)
{
    if (connection && !protobuf_screencast->has_error() &&
        protobuf_screencast->has_buffer_stream() && protobuf_screencast->buffer_stream().has_buffer())
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        try
        {
            buffer_stream = connection->make_consumer_stream(protobuf_screencast->buffer_stream());
        }
        catch (...)
        {
            callback(nullptr, context);
            create_screencast_wait_handle.result_received();
            return;
        }
    }

    callback(this, context);
    create_screencast_wait_handle.result_received();
}

void MirScreencast::released(
    MirScreencastCallback callback, void* context)
{
    callback(this, context);

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        if (buffer_stream)
            connection->release_consumer_stream(buffer_stream.get());
        buffer_stream.reset();
    }
    release_wait_handle.result_received();
}

MirBufferStream* MirScreencast::get_buffer_stream()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return buffer_stream.get();
}

void MirScreencast::screencast_done(ScreencastRequest* request)
{
    auto const status = request->response.has_error() ? mir_screencast_error_failure : mir_screencast_success;

    if (status == mir_screencast_error_failure)
        protobuf_screencast->set_error(request->response.error());

    request->available_callback(status, reinterpret_cast<MirBuffer*>(request->buffer), request->available_context);

    std::unique_lock<decltype(mutex)> lk(mutex);
    auto it = std::find_if(requests.begin(), requests.end(),
        [&request] (auto const& it) { return it.get() == request; } );
    if (it != requests.end())
        requests.erase(it);
}

void MirScreencast::screencast_to_buffer(
    mcl::MirBuffer* buffer,
    MirScreencastBufferCallback cb,
    void* context)
{
    if (!server) //construction with nullptr should be invalid
        BOOST_THROW_EXCEPTION(std::runtime_error("invalid screencast"));
    
    std::unique_lock<decltype(mutex)> lk(mutex);

    requests.emplace_back(std::make_unique<ScreencastRequest>(buffer, cb, context));

    mir::protobuf::ScreencastRequest request;
    request.set_buffer_id(buffer->rpc_id());
    request.mutable_id()->set_value(protobuf_screencast->screencast_id().value());

    server->screencast_to_buffer(
        &request,
        &(requests.back()->response),
        google::protobuf::NewCallback(this, &MirScreencast::screencast_done, requests.back().get()));
}
