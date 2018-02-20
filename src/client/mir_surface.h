/*
 * Copyright © 2012-2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_CLIENT_MIR_SURFACE_H_
#define MIR_CLIENT_MIR_SURFACE_H_

#include "cursor_configuration.h"
#include "mir/mir_buffer_stream.h"
#include "mir_wait_handle.h"
#include "frame_clock.h"
#include "rpc/mir_display_server.h"
#include "rpc/mir_display_server_debug.h"

#include "mir/client/client_platform.h"
#include "mir/frontend/surface_id.h"
#include "mir/optional_value.h"
#include "mir/geometry/dimensions.h"
#include "mir/geometry/displacement.h"
#include "mir_toolkit/common.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir/graphics/native_buffer.h"
#include "mir/time/posix_timestamp.h"

#include <memory>
#include <functional>
#include <mutex>
#include <unordered_set>

namespace mir
{
namespace dispatch
{
class ThreadedDispatcher;
}
namespace input
{
namespace receiver
{
class XKBMapper;
}
}
namespace protobuf
{
class PersistentSurfaceId;
class Surface;
class SurfaceParameters;
class SurfaceSetting;
class Void;
}
namespace client
{
namespace rpc
{
class DisplayServer;
class DisplayServerDebug;
}

class ClientBuffer;
class MirBufferStreamFactory;

struct MemoryRegion;
}
}

struct ContentInfo
{
    mir::geometry::Displacement displacement;
    int stream_id;
    mir::optional_value<mir::geometry::Size> size;
};

struct MirPersistentId
{
public:
    MirPersistentId(std::string const& string_id);

    std::string const& as_string();

private:
    std::string const string_id;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

struct MirSurfaceSpec
{
    MirSurfaceSpec();
    MirSurfaceSpec(MirConnection* connection, int width, int height, MirPixelFormat format);
    MirSurfaceSpec(MirConnection* connection, MirWindowParameters const& params);

    struct AspectRatio { unsigned width; unsigned height; };

    // Required parameters
    MirConnection* connection{nullptr};

    // Optional parameters
    mir::optional_value<int> width;
    mir::optional_value<int> height;
    mir::optional_value<MirPixelFormat> pixel_format;
    mir::optional_value<MirBufferUsage> buffer_usage;

    mir::optional_value<std::string> surface_name;
    mir::optional_value<uint32_t> output_id;

    mir::optional_value<MirWindowType> type;
    mir::optional_value<MirWindowState> state;
    mir::optional_value<MirOrientationMode> pref_orientation;

    mir::optional_value<MirWindow*> parent;
    std::shared_ptr<MirPersistentId> parent_id;
    mir::optional_value<MirRectangle> aux_rect;
    mir::optional_value<MirEdgeAttachment> edge_attachment;
    mir::optional_value<MirPlacementHints> placement_hints;
    mir::optional_value<MirPlacementGravity> surface_placement_gravity;
    mir::optional_value<MirPlacementGravity> aux_rect_placement_gravity;
    mir::optional_value<int> aux_rect_placement_offset_x;
    mir::optional_value<int> aux_rect_placement_offset_y;

    mir::optional_value<int> min_width;
    mir::optional_value<int> min_height;
    mir::optional_value<int> max_width;
    mir::optional_value<int> max_height;
    mir::optional_value<int> width_inc;
    mir::optional_value<int> height_inc;
    mir::optional_value<AspectRatio> min_aspect;
    mir::optional_value<AspectRatio> max_aspect;
    mir::optional_value<std::vector<ContentInfo>> streams;
    mir::optional_value<std::vector<MirRectangle>> input_shape;
    mir::optional_value<bool> confine_pointer;

    struct EventHandler
    {
        MirWindowEventCallback callback;
        void* context;
    };
    mir::optional_value<EventHandler> event_handler;
    mir::optional_value<MirShellChrome> shell_chrome;
    
    mir::optional_value<std::string> cursor_name;
    struct RenderSurfaceCursor
    {
        mir::frontend::BufferStreamId id;
        mir::geometry::Point hotspot;
    };
    mir::optional_value<RenderSurfaceCursor> rendersurface_cursor;
};

struct MirSurface
{
public:
    MirSurface(MirSurface const &) = delete;
    MirSurface& operator=(MirSurface const &) = delete;

    MirSurface(
        std::string const& error,
        MirConnection *allocating_connection,
        mir::frontend::SurfaceId surface_id,
        std::shared_ptr<MirWaitHandle> const& handle);

    MirSurface(
        MirConnection *allocating_connection,
        mir::client::rpc::DisplayServer& server,
        mir::client::rpc::DisplayServerDebug* debug,
        std::shared_ptr<MirBufferStream> const& buffer_stream,
        MirWindowSpec const& spec, mir::protobuf::Surface const& surface_proto,
        std::shared_ptr<MirWaitHandle> const& handle);

    ~MirSurface();

    MirWindowParameters get_parameters() const;
    char const * get_error_message();
    int id() const;

    MirWaitHandle* configure(MirWindowAttrib a, int value);

    // TODO: Some sort of extension mechanism so that this can be moved
    //       out into a separate class in the libmirclient-debug DSO.
    bool translate_to_screen_coordinates(int x, int y,
                                         int* screen_x, int* screen_y);
    
    // Non-blocking
    int attrib(MirWindowAttrib a) const;

    MirOrientation get_orientation() const;
    MirWaitHandle* set_preferred_orientation(MirOrientationMode mode);

    void raise_surface(MirCookie const* cookie);
    void request_user_move(MirCookie const* cookie);
    void request_user_resize(MirCookie const* cookie);
    void request_drag_and_drop(MirCookie const* cookie);
    void set_drag_and_drop_start_handler(std::function<void(MirWindowEvent const*)> const& callback);

    MirWaitHandle* configure_cursor(MirCursorConfiguration const* cursor);

    void set_event_handler(MirWindowEventCallback callback,
                           void* context);
    void handle_event(MirEvent& e);

    void request_and_wait_for_configure(MirWindowAttrib a, int value);

    MirBufferStream* get_buffer_stream();

    MirWaitHandle* modify(MirWindowSpec const& changes);

    static bool is_valid(MirSurface* query);

    MirWaitHandle* request_persistent_id(MirWindowIdCallback callback, void* context);
    MirConnection* connection() const;

    std::shared_ptr<mir::client::FrameClock> get_frame_clock() const;
    std::shared_ptr<mir::input::receiver::XKBMapper> get_keymapper() const;

private:
    std::mutex mutable mutex; // Protects all members of *this

    void configure_frame_clock();
    void on_configured();
    void on_cursor_configured();
    void acquired_persistent_id(MirWindowIdCallback callback, void* context);
    void request_operation(MirCookie const* cookie, mir::protobuf::RequestOperation operation) const;

    mir::client::rpc::DisplayServer* const server{nullptr};
    mir::client::rpc::DisplayServerDebug* const debug{nullptr};
    std::unique_ptr<mir::protobuf::Surface> const surface;
    std::unique_ptr<mir::protobuf::PersistentSurfaceId> const persistent_id;
    std::string const error_message;
    std::string const name;
    std::unique_ptr<mir::protobuf::Void> const void_response;

    void on_modified();
    MirWaitHandle modify_wait_handle;
    std::unique_ptr<mir::protobuf::Void> const modify_result;

    MirConnection* const connection_;

    MirWaitHandle configure_wait_handle;
    MirWaitHandle configure_cursor_wait_handle;
    MirWaitHandle persistent_id_wait_handle;

    //Deprecated functions can cause MirSurfaces to be created with a default stream
    std::shared_ptr<MirBufferStream> default_stream;
    typedef std::unordered_set<std::shared_ptr<MirBufferStream>> StreamSet;
    StreamSet streams;
    std::shared_ptr<mir::input::receiver::XKBMapper> const keymapper;

    std::unique_ptr<mir::protobuf::SurfaceSetting> const configure_result;

    // Cache of latest SurfaceSettings returned from the server
    int attrib_cache[mir_window_attribs];
    MirOrientation orientation = mir_orientation_normal;

    std::shared_ptr<mir::client::FrameClock> const frame_clock;

    std::function<void(MirEvent const*)> handle_event_callback;
    std::function<void(MirWindowEvent const*)> handle_drag_and_drop_start_callback = [](auto){};

    std::shared_ptr<mir::dispatch::ThreadedDispatcher> input_thread;

    //a bit batty, but the creation handle has to exist for as long as the MirSurface does,
    //as we don't really manage the lifetime of MirWaitHandle sensibly.
    std::shared_ptr<MirWaitHandle> const creation_handle;
    mir::geometry::Size size;
    MirPixelFormat format;
    MirBufferUsage usage;
    uint32_t output_id;
};

#pragma GCC diagnostic pop

#endif /* MIR_CLIENT_PRIVATE_MIR_WAIT_HANDLE_H_ */
