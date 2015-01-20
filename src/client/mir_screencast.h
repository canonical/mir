/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

#include "mir/egl_native_surface.h"
#include "mir_wait_handle.h"
#include "mir_toolkit/client_types.h"
#include "mir_protobuf.pb.h"
#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"

#include <EGL/eglplatform.h>

namespace mir
{
namespace protobuf { class DisplayServer; }
namespace client
{
class ClientBufferStreamFactory;
class ClientBufferStream;
}
}

struct MirScreencast : public mir::client::EGLNativeSurface
{
public:
    MirScreencast(
        mir::geometry::Rectangle const& region,
        mir::geometry::Size const& size,
        MirPixelFormat pixel_format,
        mir::protobuf::DisplayServer& server,
        std::shared_ptr<mir::client::ClientBufferStreamFactory> const& buffer_stream_factory,
        mir_screencast_callback callback, void* context);

    MirWaitHandle* creation_wait_handle();
    bool valid();

    MirWaitHandle* release(
        mir_screencast_callback callback, void* context);

    MirWaitHandle* next_buffer(
        mir_screencast_callback callback, void* context);

    EGLNativeWindowType egl_native_window();

    /* mir::client::EGLNativeSurface */
    MirSurfaceParameters get_parameters() const;
    std::shared_ptr<mir::client::ClientBuffer> get_current_buffer();
    void request_and_wait_for_next_buffer();
    void request_and_wait_for_configure(MirSurfaceAttrib a, int value);

private:
    void screencast_created(
        mir_screencast_callback callback, void* context);
    void released(
        mir_screencast_callback callback, void* context);

    mir::protobuf::DisplayServer& server;
    mir::geometry::Size const output_size;
    std::shared_ptr<mir::client::ClientBufferStreamFactory> const buffer_stream_factory;
    
    std::shared_ptr<mir::client::ClientBufferStream> buffer_stream;

    mir::protobuf::Screencast protobuf_screencast;
    mir::protobuf::Void protobuf_void;

    MirWaitHandle create_screencast_wait_handle;
    MirWaitHandle release_wait_handle;
};

#endif /* MIR_CLIENT_MIR_SCREENCAST_H_ */
