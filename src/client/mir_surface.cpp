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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_surface.h"
#include "mir_connection.h"
#include "cursor_configuration.h"
#include "make_protobuf_object.h"
#include "mir_protobuf.pb.h"
#include "connection_surface_map.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_blob.h"
#include "mir/frontend/client_constants.h"
#include "mir/client/client_buffer.h"
#include "mir/mir_buffer_stream.h"
#include "mir/dispatch/threaded_dispatcher.h"
#include "mir/input/xkb_mapper.h"
#include "mir/cookie/cookie.h"
#include "mir_cookie.h"
#include "mir/time/posix_timestamp.h"
#include "mir/events/surface_event.h"

#include <cassert>
#include <unistd.h>

#include <boost/exception/diagnostic_information.hpp>

namespace geom = mir::geometry;
namespace mcl = mir::client;
namespace mclr = mir::client::rpc;
namespace mi = mir::input;
namespace mircv = mi::receiver;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;
namespace md = mir::dispatch;

using mir::client::FrameClock;

namespace
{
std::mutex handle_mutex;
std::unordered_set<MirWindow*> valid_surfaces;

void apply_device_state(MirEvent const& event, mi::KeyMapper& keymapper)
{
    auto device_state = mir_event_get_input_device_state_event(&event);
    std::vector<uint32_t> keys;

    for (size_t index = 0, end_index = mir_input_device_state_event_device_count(device_state);
         index != end_index; ++index)
    {
        auto device_id = mir_input_device_state_event_device_id(device_state, index);
        auto key_count = mir_input_device_state_event_device_pressed_keys_count(device_state, index);
        for (decltype(key_count) i = 0; i != key_count; ++i)
            keys.push_back(
                mir_input_device_state_event_device_pressed_keys_for_index(device_state, index, i));

        keymapper.set_key_state(device_id, keys);
    }
}

}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

MirPersistentId::MirPersistentId(std::string const& string_id)
    : string_id{string_id}
{
}

std::string const& MirPersistentId::as_string()
{
    return string_id;
}

MirSurfaceSpec::MirSurfaceSpec(
    MirConnection* connection, int width, int height, MirPixelFormat format)
    : connection{connection},
      width{width},
      height{height},
      pixel_format{format},
      buffer_usage{mir_buffer_usage_hardware}
{
}

MirSurfaceSpec::MirSurfaceSpec(MirConnection* connection, MirWindowParameters const& params)
    : connection{connection},
      width{params.width},
      height{params.height},
      pixel_format{params.pixel_format},
      buffer_usage{params.buffer_usage}
{
    type = mir_window_type_normal;
    if (params.output_id != mir_display_output_id_invalid)
    {
        output_id = params.output_id;
        state = mir_window_state_fullscreen;
    }
}

MirSurfaceSpec::MirSurfaceSpec() = default;

MirSurface::MirSurface(
    std::string const& error,
    MirConnection* conn,
    mir::frontend::SurfaceId id,
    std::shared_ptr<MirWaitHandle> const& handle) :
    surface{mcl::make_protobuf_object<mir::protobuf::Surface>()},
    connection_(conn),
    frame_clock(std::make_shared<FrameClock>()),
    creation_handle(handle)
{
    surface->set_error(error);
    surface->mutable_id()->set_value(id.as_value());

    std::lock_guard<decltype(handle_mutex)> lock(handle_mutex);
    valid_surfaces.insert(this);

    configure_frame_clock();
}

MirSurface::MirSurface(
    MirConnection *allocating_connection,
    mclr::DisplayServer& the_server,
    mclr::DisplayServerDebug* debug,
    std::shared_ptr<MirBufferStream> const& buffer_stream,
    MirWindowSpec const& spec,
    mir::protobuf::Surface const& surface_proto,
    std::shared_ptr<MirWaitHandle> const& handle)
    : server{&the_server},
      debug{debug},
      surface{mcl::make_protobuf_object<mir::protobuf::Surface>(surface_proto)},
      persistent_id{mcl::make_protobuf_object<mir::protobuf::PersistentSurfaceId>()},
      name{spec.surface_name.is_set() ? spec.surface_name.value() : ""},
      void_response{mcl::make_protobuf_object<mir::protobuf::Void>()},
      modify_result{mcl::make_protobuf_object<mir::protobuf::Void>()},
      connection_(allocating_connection),
      default_stream(buffer_stream),
      keymapper(std::make_shared<mircv::XKBMapper>()),
      configure_result{mcl::make_protobuf_object<mir::protobuf::SurfaceSetting>()},
      frame_clock(std::make_shared<FrameClock>()),
      creation_handle(handle),
      size({surface_proto.width(), surface_proto.height()}),
      format(static_cast<MirPixelFormat>(surface_proto.pixel_format())),
      usage(static_cast<MirBufferUsage>(surface_proto.buffer_usage())),
      output_id(spec.output_id.is_set() ? spec.output_id.value() : static_cast<uint32_t>(mir_display_output_id_invalid))
{
    if (default_stream)
        streams.insert(default_stream);

    if (spec.streams.is_set())
    {
        auto& map = connection_->connection_surface_map();
        auto const& spec_streams = spec.streams.value();
        for (auto& ci : spec_streams)
        {
            mir::frontend::BufferStreamId id(ci.stream_id);
            if (auto bs = map->stream(id))
                streams.insert(bs);
        }
    }

    for (auto& s : streams)
        s->adopted_by(this);

    for(int i = 0; i < surface_proto.attributes_size(); i++)
    {
        auto const& attrib = surface_proto.attributes(i);
        attrib_cache[attrib.attrib()] = attrib.ivalue();
    }

    if (spec.event_handler.is_set())
    {
        handle_event_callback = std::bind(
            spec.event_handler.value().callback,
            this,
            std::placeholders::_1,
            spec.event_handler.value().context);
    }

    std::lock_guard<decltype(handle_mutex)> lock(handle_mutex);
    valid_surfaces.insert(this);

    configure_frame_clock();
}

MirSurface::~MirSurface()
{
    StreamSet old_streams;

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        old_streams = std::move(streams);
    }

    // Do callbacks without holding the lock
    for (auto const& s : old_streams)
        s->unadopted_by(this);

    {
        std::lock_guard<decltype(handle_mutex)> lock(handle_mutex);
        valid_surfaces.erase(this);
    }

    std::lock_guard<decltype(mutex)> lock(mutex);

    input_thread.reset();

    for (auto i = 0, end = surface->fd_size(); i != end; ++i)
        close(surface->fd(i));
}

void MirSurface::configure_frame_clock()
{
    /*
     * TODO: Implement frame_clock->set_resync_callback(...) when IPC to get
     *       timestamps from the server exists.
     *       Until we do, client-side vsync will perform suboptimally (it is
     *       randomly up to one frame out of phase with the real display).
     *       However even while it's suboptimal it's dramatically lower latency
     *       than the old approach and still totally eliminates nesting lag.
     */
}

MirWindowParameters MirSurface::get_parameters() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return {name.c_str(), size.width.as_int(), size.height.as_int(), format, usage, output_id};
}

char const * MirSurface::get_error_message()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (surface->has_error())
    {
        return surface->error().c_str();
    }
    return error_message.c_str();
}

int MirSurface::id() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return surface->id().value();
}

bool MirSurface::is_valid(MirSurface* query)
{
    std::lock_guard<decltype(handle_mutex)> lock(handle_mutex);

    if (valid_surfaces.count(query))
        return !query->surface->has_error();

    return false;
}

void MirSurface::acquired_persistent_id(MirWindowIdCallback callback, void* context)
{
    if (!persistent_id->has_error())
    {
        callback(this, new MirPersistentId{persistent_id->value()}, context);
    }
    else
    {
        callback(this, nullptr, context);
    }
    persistent_id_wait_handle.result_received();
}

MirWaitHandle* MirSurface::request_persistent_id(MirWindowIdCallback callback, void* context)
{
    std::lock_guard<decltype(mutex)> lock{mutex};

    if (persistent_id->has_value())
    {
        callback(this, new MirPersistentId{persistent_id->value()}, context);
        return nullptr;
    }

    persistent_id_wait_handle.expect_result();
    try
    {
        server->request_persistent_surface_id(&surface->id(), persistent_id.get(), gp::NewCallback(this, &MirSurface::acquired_persistent_id, callback, context));
    }
    catch (std::exception const& ex)
    {
        surface->set_error(std::string{"Failed to acquire a persistent ID from the server: "} +
                          boost::diagnostic_information(ex));
    }
    return &persistent_id_wait_handle;
}

MirWaitHandle* MirSurface::configure_cursor(MirCursorConfiguration const* cursor)
{
    mp::CursorSetting setting;

    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        setting.mutable_surfaceid()->CopyFrom(surface->id());
        if (cursor)
        {
            if (cursor->surface || cursor->stream)
            {
                auto id = cursor->surface ? cursor->surface->stream_id().as_value() :
                                            cursor->stream->rpc_id().as_value();
                setting.mutable_buffer_stream()->set_value(id);
                setting.set_hotspot_x(cursor->hotspot_x);
                setting.set_hotspot_y(cursor->hotspot_y);
            }
            else if (cursor->name != mir_disabled_cursor_name)
            {
                setting.set_name(cursor->name.c_str());
            }
        }
    }
    
    configure_cursor_wait_handle.expect_result();
    server->configure_cursor(&setting, void_response.get(),
        google::protobuf::NewCallback(this, &MirSurface::on_cursor_configured));
    
    return &configure_cursor_wait_handle;
}

MirWaitHandle* MirSurface::configure(MirWindowAttrib at, int value)
{
    // TODO: This is obviously strange. It should be
    // possible to eliminate it in the second phase of buffer
    // stream where the existing MirSurface swap interval functions
    // may be deprecated in terms of mir_buffer_stream_ alternatives
    if ((at == mir_window_attrib_swapinterval) && default_stream)
    {
        default_stream->set_swap_interval(value);
        return &configure_wait_handle;
    }

    std::unique_lock<decltype(mutex)> lock(mutex);

    mp::SurfaceSetting setting;
    setting.mutable_surfaceid()->CopyFrom(surface->id());
    setting.set_attrib(at);
    setting.set_ivalue(value);
    lock.unlock();

    configure_wait_handle.expect_result();
    server->configure_surface(&setting, configure_result.get(),
              google::protobuf::NewCallback(this, &MirSurface::on_configured));

    return &configure_wait_handle;
}

namespace
{
void signal_response_received(MirWaitHandle* handle)
{
    handle->result_received();
}
}

bool MirSurface::translate_to_screen_coordinates(int x, int y,
                                                 int *screen_x, int *screen_y)
{
    if (!debug)
    {
        return false;
    }

    mp::CoordinateTranslationRequest request;

    request.set_x(x);
    request.set_y(y);
    *request.mutable_surfaceid() = surface->id();
    mp::CoordinateTranslationResponse response;

    MirWaitHandle signal;
    signal.expect_result();

    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        debug->translate_surface_to_screen(
            &request,
            &response,
            google::protobuf::NewCallback(&signal_response_received, &signal));
    }

    signal.wait_for_one();

    *screen_x = response.x();
    *screen_y = response.y();
    return !response.has_error();
}

void MirSurface::on_configured()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (configure_result->has_surfaceid() &&
        configure_result->surfaceid().value() == surface->id().value() &&
        configure_result->has_attrib())
    {
        int a = configure_result->attrib();

        switch (a)
        {
        case mir_window_attrib_type:
        case mir_window_attrib_state:
        case mir_window_attrib_focus:
        case mir_window_attrib_dpi:
        case mir_window_attrib_preferred_orientation:
            if (configure_result->has_ivalue())
                attrib_cache[a] = configure_result->ivalue();
            else
                assert(configure_result->has_error());
            break;
        default:
            assert(false);
            break;
        }

        configure_wait_handle.result_received();
    }
}

void MirSurface::on_cursor_configured()
{
    configure_cursor_wait_handle.result_received();
}


int MirSurface::attrib(MirWindowAttrib at) const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (at == mir_window_attrib_swapinterval)
    {
        if (default_stream)
            return default_stream->swap_interval();
        else // Surface creation is not finalized
            return 1;
    }

    return attrib_cache[at];
}

void MirSurface::set_event_handler(MirWindowEventCallback callback,
                                   void* context)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    input_thread.reset();
    handle_event_callback = [](auto){};

    if (callback)
    {
        handle_event_callback = std::bind(callback, this,
                                          std::placeholders::_1,
                                          context);
    }
}

void MirSurface::handle_event(MirEvent& e)
{
    std::unique_lock<decltype(mutex)> lock(mutex);

    switch (mir_event_get_type(&e))
    {
    case mir_event_type_window:
    {
        auto sev = mir_event_get_window_event(&e);
        auto a = mir_window_event_get_attribute(sev);
        if (a < mir_window_attribs)
        {
            attrib_cache[a] = mir_window_event_get_attribute_value(sev);
        }
        else
        {
            handle_drag_and_drop_start_callback(sev);
            return;
        }
        break;
    }
    case mir_event_type_input:
        keymapper->map_event(e);
        break;
    case mir_event_type_input_device_state:
        apply_device_state(e, *keymapper);
        break;
    case mir_event_type_orientation:
        orientation = mir_orientation_event_get_direction(mir_event_get_orientation_event(&e));
        break;
    case mir_event_type_keymap:
    {
        char const* buffer = nullptr;
        size_t length = 0;
        auto keymap_event = mir_event_get_keymap_event(&e);
        mir_keymap_event_get_keymap_buffer(keymap_event, &buffer, &length);
        keymapper->set_keymap_for_all_devices(buffer, length);
        break;
    }
    case mir_event_type_resize:
    {
        auto resize_event = mir_event_get_resize_event(&e);
        size = { mir_resize_event_get_width(resize_event), mir_resize_event_get_height(resize_event) };
        if (default_stream)
            default_stream->set_size(size);
        break;
    }
    case mir_event_type_window_output:
    {
        /*
         * We set the frame clock according to the (maximum) refresh rate of
         * the display. Note that we don't do the obvious thing and measure
         * previous vsyncs mainly because that's a very unreliable predictor
         * of the desired maximum refresh rate in the case of GSync/FreeSync
         * and panel self-refresh. You can't assume the previous frame
         * represents the display running at full native speed and so should
         * not measure it. Instead we conveniently have
         * mir_window_output_event_get_refresh_rate that tells us the full
         * native speed of the most relevant output...
         */
        auto soevent = mir_event_get_surface_output_event(&e);
        auto rate = mir_surface_output_event_get_refresh_rate(soevent);
        if (rate > 10.0)  // should be >0, but 10 to workaround LP: #1639725
        {
            std::chrono::nanoseconds const ns(
                static_cast<long>(1000000000L / rate));
            frame_clock->set_period(ns);
        }
        /* else: The graphics driver has not provided valid timing so we will
         *       default to swap interval 0 behaviour.
         *       Or would people prefer some different fallback?
         *         - Re-enable server-side vsync?
         *         - Rate limit to 60Hz?
         *         - Just fix LP: #1639725?
         */
        break;
    }
    default:
        break;
    };

    if (handle_event_callback)
    {
        auto callback = handle_event_callback;
        lock.unlock();
        callback(&e);
    }
}

void MirSurface::request_and_wait_for_configure(MirWindowAttrib a, int value)
{
    configure(a, value)->wait_for_all();
}

MirOrientation MirSurface::get_orientation() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return orientation;
}

MirWaitHandle* MirSurface::set_preferred_orientation(MirOrientationMode mode)
{
    return configure(mir_window_attrib_preferred_orientation, mode);
}

void MirSurface::raise_surface(MirCookie const* cookie)
{
    request_operation(cookie, mp::RequestOperation::MAKE_ACTIVE);
}

void MirSurface::request_user_move(MirCookie const* cookie)
{
    request_operation(cookie, mp::RequestOperation::USER_MOVE);
}

void MirSurface::request_user_resize(MirResizeEdge edge, MirCookie const* cookie)
{
    request_operation(cookie, mp::RequestOperation::USER_RESIZE, edge);
}

void MirSurface::request_drag_and_drop(MirCookie const* cookie)
{
    request_operation(cookie, mp::RequestOperation::START_DRAG_AND_DROP);
}

void MirSurface::request_operation(MirCookie const* cookie, mir::protobuf::RequestOperation operation) const
{
    request_operation(cookie, operation, mir::optional_value<uint32_t>{});
}

void MirSurface::request_operation(MirCookie const* cookie, mir::protobuf::RequestOperation operation, mir::optional_value<uint32_t> hint) const
{
    mir::protobuf::RequestWithAuthority request;
    request.set_operation(operation);

    std::unique_lock<decltype(mutex)> lock(mutex);
    request.mutable_surface_id()->set_value(surface->id().value());

    auto const event_authority = request.mutable_authority();

    event_authority->set_cookie(cookie->cookie().data(), cookie->size());

    if (hint.is_set())
        request.set_hint(hint.value());

    server->request_operation(
        &request,
        void_response.get(),
        google::protobuf::NewCallback(google::protobuf::DoNothing));
}

void MirSurface::set_drag_and_drop_start_handler(std::function<void(MirWindowEvent const*)> const& callback)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    handle_drag_and_drop_start_callback = callback;
}

MirBufferStream* MirSurface::get_buffer_stream()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    
    return default_stream.get();
}

void MirSurface::on_modified()
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        if (modify_result->has_error())
        {
            // TODO return errors like lp:~vanvugt/mir/wait-result
        }
    }
    modify_wait_handle.result_received();
}

MirWaitHandle* MirSurface::modify(MirWindowSpec const& spec)
{
    mp::SurfaceModifications mods;

    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        mods.mutable_surface_id()->set_value(surface->id().value());
    }

    auto const surface_specification = mods.mutable_surface_specification();

    #define COPY_IF_SET(field)\
        if (spec.field.is_set())\
        surface_specification->set_##field(spec.field.value())

    COPY_IF_SET(width);
    COPY_IF_SET(height);
    COPY_IF_SET(pixel_format);
    COPY_IF_SET(buffer_usage);
    // name is a special case (below)
    COPY_IF_SET(output_id);
    COPY_IF_SET(type);
    COPY_IF_SET(state);
    // preferred_orientation is a special case (below)
    // parent_id is a special case (below)
    // aux_rect is a special case (below)
    COPY_IF_SET(edge_attachment);
    COPY_IF_SET(aux_rect_placement_gravity);
    COPY_IF_SET(surface_placement_gravity);
    COPY_IF_SET(placement_hints);
    COPY_IF_SET(aux_rect_placement_offset_x);
    COPY_IF_SET(aux_rect_placement_offset_y);
    COPY_IF_SET(min_width);
    COPY_IF_SET(min_height);
    COPY_IF_SET(max_width);
    COPY_IF_SET(max_height);
    COPY_IF_SET(width_inc);
    COPY_IF_SET(height_inc);
    COPY_IF_SET(shell_chrome);
    COPY_IF_SET(confine_pointer);
    COPY_IF_SET(cursor_name);
    // min_aspect is a special case (below)
    // max_aspect is a special case (below)
    #undef COPY_IF_SET

    if (spec.surface_name.is_set())
        surface_specification->set_name(spec.surface_name.value());

    if (spec.pref_orientation.is_set())
        surface_specification->set_preferred_orientation(spec.pref_orientation.value());

    if (spec.parent.is_set() && spec.parent.value())
        surface_specification->set_parent_id(spec.parent.value()->id());

    if (spec.parent_id)
    {
        auto id = surface_specification->mutable_parent_persistent_id();
        id->set_value(spec.parent_id->as_string());
    }

    if (spec.aux_rect.is_set())
    {
        auto const rect = surface_specification->mutable_aux_rect();
        auto const& value = spec.aux_rect.value();
        rect->set_left(value.left);
        rect->set_top(value.top);
        rect->set_width(value.width);
        rect->set_height(value.height);
    }

    if (spec.min_aspect.is_set())
    {
        auto const aspect = surface_specification->mutable_min_aspect();
        aspect->set_width(spec.min_aspect.value().width);
        aspect->set_height(spec.min_aspect.value().height);
    }

    if (spec.max_aspect.is_set())
    {
        auto const aspect = surface_specification->mutable_max_aspect();
        aspect->set_width(spec.max_aspect.value().width);
        aspect->set_height(spec.max_aspect.value().height);
    }

    if (spec.streams.is_set())
    {
        std::shared_ptr<mir::client::ConnectionSurfaceMap> map;
        StreamSet old_streams, new_streams;

        /*
         * Note that we are updating our local copy of 'streams' before the
         * server has updated its copy. That's mainly because we have never yet
         * implemented any kind of client-side error handling for when
         * modify_surface fails and is rejected. There's a prototype started
         * along those lines in lp:~vanvugt/mir/adoption, but it does not seem
         * to be important to block on...
         *   The worst case is still perfectly acceptable: That is if the server
         * rejects the surface spec our local copy of streams is wrong. That's
         * a harmless consequence which just means the stream is now
         * synchronized to the output the client intended even though the server
         * is refusing to display it.
         */
        {
            std::lock_guard<decltype(mutex)> lock(mutex);
            default_stream = nullptr;
            old_streams = std::move(streams);
            streams.clear();  // Leave the container valid before we unlock
            map = connection_->connection_surface_map();
        }

        for(auto const& stream : spec.streams.value())
        {
            auto const new_stream = surface_specification->add_stream();
            new_stream->set_displacement_x(stream.displacement.dx.as_int());
            new_stream->set_displacement_y(stream.displacement.dy.as_int());
            new_stream->mutable_id()->set_value(stream.stream_id);
            if (stream.size.is_set())
            {
                new_stream->set_width(stream.size.value().width.as_int());
                new_stream->set_height(stream.size.value().height.as_int());
            }

            mir::frontend::BufferStreamId id(stream.stream_id);
            if (auto bs = map->stream(id))
                new_streams.insert(bs);
            /* else: we could implement stream ID validation here, which we've
             * never had before.
             * The only problem with that plan is that we have some tests
             * that rely on the ability to pass in invalid stream IDs so those
             * need fixing.
             */
        }

        // Worth optimizing and avoiding the intersection of both sets?....
        for (auto const& old_stream : old_streams)
            old_stream->unadopted_by(this);
        for (auto const& new_stream : new_streams)
            new_stream->adopted_by(this);
        // ... probably not, since we can std::move(new_streams) this way...
        {
            std::unique_lock<decltype(mutex)> lock(mutex);
            streams = std::move(new_streams);
        }
    }

    if (spec.input_shape.is_set())
    {
        for (auto const& rect : spec.input_shape.value())
        {
            auto const new_shape = surface_specification->add_input_shape();
            new_shape->set_left(rect.left);
            new_shape->set_top(rect.top);
            new_shape->set_width(rect.width);
            new_shape->set_height(rect.height);
        }
    }

    if (spec.rendersurface_cursor.is_set())
    {
        auto const rs_cursor = spec.rendersurface_cursor.value();
        surface_specification->mutable_cursor_id()->set_value(rs_cursor.id.as_value());
        surface_specification->set_hotspot_x(rs_cursor.hotspot.x.as_int());
        surface_specification->set_hotspot_y(rs_cursor.hotspot.y.as_int());
    }

    if (spec.event_handler.is_set())
    {
        set_event_handler(
            spec.event_handler.value().callback,
            spec.event_handler.value().context);
    }

    modify_wait_handle.expect_result();
    server->modify_surface(&mods, modify_result.get(),
              google::protobuf::NewCallback(this, &MirSurface::on_modified));

    return &modify_wait_handle;
}

MirConnection* MirSurface::connection() const
{
    return connection_;
}

std::shared_ptr<FrameClock> MirSurface::get_frame_clock() const
{
    return frame_clock;
}

std::shared_ptr<mir::input::receiver::XKBMapper> MirSurface::get_keymapper() const
{
    return keymapper;
}

#pragma GCC diagnostic pop
