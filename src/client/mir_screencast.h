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

#include "mir_client_surface.h"
#include "mir_wait_handle.h"
#include "client_buffer_depository.h"
#include "mir_toolkit/client_types.h"
#include "mir_protobuf.pb.h"
#include "mir/geometry/size.h"

#include <EGL/eglplatform.h>

namespace mir
{
namespace protobuf { class DisplayServer; }
namespace client
{
class ClientBufferFactory;
class EGLNativeWindowFactory;
}
}

struct MirScreencast : public mir::client::ClientSurface
{
public:
    MirScreencast(
        MirDisplayOutput const& output,
        mir::protobuf::DisplayServer& server,
        std::shared_ptr<mir::client::EGLNativeWindowFactory> const& egl_native_window_factory,
        std::shared_ptr<mir::client::ClientBufferFactory> const& factory,
        mir_screencast_callback callback, void* context);

    MirWaitHandle* creation_wait_handle();

    MirWaitHandle* release(
        mir_screencast_callback callback, void* context);

    MirWaitHandle* next_buffer(
        mir_screencast_callback callback, void* context);

    EGLNativeWindowType egl_native_window();

    /* mir::client::ClientSurface */
    MirSurfaceParameters get_parameters() const;
    std::shared_ptr<mir::client::ClientBuffer> get_current_buffer();
    void request_and_wait_for_next_buffer();
    void request_and_wait_for_configure(MirSurfaceAttrib a, int value);

private:
    void process_buffer(mir::protobuf::Buffer const& buffer);
    void screencast_created(
        mir_screencast_callback callback, void* context);
    void released(
        mir_screencast_callback callback, void* context);
    void next_buffer_received(
        mir_screencast_callback callback, void* context);

    mir::protobuf::DisplayServer& server;
    uint32_t const output_id;
    mir::geometry::Size const output_size;
    MirPixelFormat const output_format;
    std::shared_ptr<mir::client::EGLNativeWindowFactory> const egl_native_window_factory;
    mir::client::ClientBufferDepository buffer_depository;
    std::shared_ptr<EGLNativeWindowType> egl_native_window_;

    mir::protobuf::Screencast protobuf_screencast;
    mir::protobuf::Buffer protobuf_buffer;
    mir::protobuf::Void protobuf_void;

    MirWaitHandle create_screencast_wait_handle;
    MirWaitHandle release_wait_handle;
    MirWaitHandle next_buffer_wait_handle;
};

#endif /* MIR_CLIENT_MIR_SCREENCAST_H_ */
