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

#ifndef MIR_CLIENT_MIR_SCREENCAST_H_
#define MIR_CLIENT_MIR_SCREENCAST_H_

#include "mir_wait_handle.h"
#include "mir_toolkit/mir_screencast.h"
#include "mir/optional_value.h"
#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"
#include "mir_protobuf.pb.h"

#include <EGL/eglplatform.h>

#include <memory>

namespace mir
{
namespace protobuf
{
class Screencast;
class Void;
}
namespace client
{
class MirBuffer;
namespace rpc
{
class DisplayServer;
}
class MirBufferStreamFactory;
}
}

struct MirScreencastSpec
{
    MirScreencastSpec();
    MirScreencastSpec(MirConnection* connection);
    MirScreencastSpec(MirConnection* connection, MirScreencastParameters const& params);

    // Required parameters
    MirConnection* connection{nullptr};

    // Optional parameters
    mir::optional_value<unsigned int> width;
    mir::optional_value<unsigned int> height;
    mir::optional_value<MirPixelFormat> pixel_format;
    mir::optional_value<MirRectangle> capture_region;

    mir::optional_value<MirMirrorMode> mirror_mode;
    mir::optional_value<unsigned int> num_buffers;
};

struct MirScreencast
{
public:
    MirScreencast(std::string const& error);
    MirScreencast(
        MirScreencastSpec const& spec,
        mir::client::rpc::DisplayServer& server,
        MirScreencastCallback callback, void* context);

    MirWaitHandle* creation_wait_handle();
    bool valid();
    char const* get_error_message();

    MirWaitHandle* release(
        MirScreencastCallback callback, void* context);

    EGLNativeWindowType egl_native_window();

    MirBufferStream* get_buffer_stream();

    void screencast_to_buffer(
        mir::client::MirBuffer* buffer,
        MirScreencastBufferCallback available_callback,
        void* available_context);

private:
    void screencast_created(
        MirScreencastCallback callback, void* context);
    void released(
        MirScreencastCallback callback, void* context);

    std::mutex mutable mutex;
    mir::client::rpc::DisplayServer* const server{nullptr};
    MirConnection* const connection{nullptr};
    std::shared_ptr<MirBufferStream> buffer_stream;

    std::unique_ptr<mir::protobuf::Screencast> const protobuf_screencast;
    std::unique_ptr<mir::protobuf::Void> const protobuf_void;

    MirWaitHandle create_screencast_wait_handle;
    MirWaitHandle release_wait_handle;

    std::string const empty_error_message;

    struct ScreencastRequest
    {
        ScreencastRequest(mir::client::MirBuffer* b, MirScreencastBufferCallback cb, void* context) :
            buffer(b),
            available_callback(cb),
            available_context(context)
        {
        }
 
        mir::client::MirBuffer* buffer;
        MirScreencastBufferCallback available_callback;
        void* available_context;
        mir::protobuf::Void response;
    };
    std::vector<std::unique_ptr<ScreencastRequest>> requests;
    void screencast_done(ScreencastRequest* request);
};

#endif /* MIR_CLIENT_MIR_SCREENCAST_H_ */
