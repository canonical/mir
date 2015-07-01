/*
 * Copyright © 2012, 2015 Canonical Ltd.
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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_surface.h"
#include "cursor_configuration.h"
#include "client_buffer_stream_factory.h"
#include "make_protobuf_object.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir/frontend/client_constants.h"
#include "mir/client_buffer.h"
#include "mir_connection.h"
#include "client_buffer_stream.h"
#include "mir/dispatch/threaded_dispatcher.h"
#include "mir/input/input_platform.h"
#include "mir/input/xkb_mapper.h"

#include <cassert>
#include <unistd.h>

#include <boost/exception/diagnostic_information.hpp>

namespace geom = mir::geometry;
namespace mcl = mir::client;
namespace mircv = mir::input::receiver;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;
namespace md = mir::dispatch;

#define SERIALIZE_OPTION_IF_SET(option, message) \
    if (option.is_set()) \
        message->set_##option(option.value());

namespace
{
std::mutex handle_mutex;
std::unordered_set<MirSurface*> valid_surfaces;
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

MirSurfaceSpec::MirSurfaceSpec(MirConnection* connection, MirSurfaceParameters const& params)
    : connection{connection},
      width{params.width},
      height{params.height},
      pixel_format{params.pixel_format},
      buffer_usage{params.buffer_usage}
{
    type = mir_surface_type_normal;
    if (params.output_id != mir_display_output_id_invalid)
    {
        output_id = params.output_id;
        state = mir_surface_state_fullscreen;
    }
}

MirSurfaceSpec::MirSurfaceSpec()
{
}

std::unique_ptr<mir::protobuf::SurfaceParameters> MirSurfaceSpec::serialize() const
{
    //std::unique_ptr<mp::SurfaceParameters> message{mp::SurfaceParameters::default_instance().New()};
    auto message = mcl::make_protobuf_object<mp::SurfaceParameters>();

    SERIALIZE_OPTION_IF_SET(width, message);
    SERIALIZE_OPTION_IF_SET(height, message);
    SERIALIZE_OPTION_IF_SET(pixel_format, message);
    SERIALIZE_OPTION_IF_SET(buffer_usage, message);
    SERIALIZE_OPTION_IF_SET(surface_name, message);
    SERIALIZE_OPTION_IF_SET(output_id, message);
    SERIALIZE_OPTION_IF_SET(type, message);
    SERIALIZE_OPTION_IF_SET(state, message);
    SERIALIZE_OPTION_IF_SET(pref_orientation, message);
    SERIALIZE_OPTION_IF_SET(edge_attachment, message);
    SERIALIZE_OPTION_IF_SET(min_width, message);
    SERIALIZE_OPTION_IF_SET(min_height, message);
    SERIALIZE_OPTION_IF_SET(max_width, message);
    SERIALIZE_OPTION_IF_SET(max_height, message);
    SERIALIZE_OPTION_IF_SET(width_inc, message);
    SERIALIZE_OPTION_IF_SET(height_inc, message);
    // min_aspect is a special case (below)
    // max_aspect is a special case (below)

    if (parent.is_set() && parent.value() != nullptr)
        message->set_parent_id(parent.value()->id());

    if (parent_id)
    {
        auto id = message->mutable_parent_persistent_id();
        id->set_value(parent_id->as_string());
    }

    if (aux_rect.is_set())
    {
        message->mutable_aux_rect()->set_left(aux_rect.value().left);
        message->mutable_aux_rect()->set_top(aux_rect.value().top);
        message->mutable_aux_rect()->set_width(aux_rect.value().width);
        message->mutable_aux_rect()->set_height(aux_rect.value().height);
    }

    if (min_aspect.is_set())
    {
        message->mutable_min_aspect()->set_width(min_aspect.value().width);
        message->mutable_min_aspect()->set_height(min_aspect.value().height);
    }

    if (max_aspect.is_set())
    {
        message->mutable_max_aspect()->set_width(max_aspect.value().width);
        message->mutable_max_aspect()->set_height(max_aspect.value().height);
    }

    if (input_shape.is_set())
    {
        for (auto const& rect : input_shape.value())
        {
            auto const new_shape = message->add_input_shape();
            new_shape->set_left(rect.left);
            new_shape->set_top(rect.top);
            new_shape->set_width(rect.width);
            new_shape->set_height(rect.height);
        }
    }

    return message;
}

MirPersistentId::MirPersistentId(std::string const& string_id)
    : string_id{string_id}
{
}

std::string const&MirPersistentId::as_string()
{
    return string_id;
}

MirSurface::MirSurface(std::string const& error)
    : surface{mcl::make_protobuf_object<mir::protobuf::Surface>()}
{
    surface->set_error(error);

    std::lock_guard<decltype(handle_mutex)> lock(handle_mutex);
    valid_surfaces.insert(this);
}

MirSurface::MirSurface(
    MirConnection *allocating_connection,
    mp::DisplayServer::Stub& the_server,
    mp::Debug::Stub* debug,
    std::shared_ptr<mcl::ClientBufferStreamFactory> const& buffer_stream_factory,
    std::shared_ptr<mircv::InputPlatform> const& input_platform,
    MirSurfaceSpec const& spec,
    mir_surface_callback callback, void * context)
    : server{&the_server},
      debug{debug},
      surface{mcl::make_protobuf_object<mir::protobuf::Surface>()},
      persistent_id{mcl::make_protobuf_object<mir::protobuf::PersistentSurfaceId>()},
      name{spec.surface_name.is_set() ? spec.surface_name.value() : ""},
      void_response{mcl::make_protobuf_object<mir::protobuf::Void>()},
      modify_result{mcl::make_protobuf_object<mir::protobuf::Void>()},
      connection(allocating_connection),
      buffer_stream_factory(buffer_stream_factory),
      input_platform(input_platform),
      keymapper(std::make_shared<mircv::XKBMapper>()),
      configure_result{mcl::make_protobuf_object<mir::protobuf::SurfaceSetting>()}
{
    for (int i = 0; i < mir_surface_attribs; i++)
        attrib_cache[i] = -1;

    auto const message = spec.serialize();
    create_wait_handle.expect_result();
    try 
    {
        server->create_surface(0, message.get(), surface.get(), gp::NewCallback(this, &MirSurface::created, callback, context));
    }
    catch (std::exception const& ex)
    {
        surface->set_error(std::string{"Error invoking create surface: "} +
                          boost::diagnostic_information(ex));
    }

    std::lock_guard<decltype(handle_mutex)> lock(handle_mutex);
    valid_surfaces.insert(this);
}

MirSurface::~MirSurface()
{
    {
        std::lock_guard<decltype(handle_mutex)> lock(handle_mutex);
        valid_surfaces.erase(this);
    }

    std::lock_guard<decltype(mutex)> lock(mutex);

    input_thread.reset();

    for (auto i = 0, end = surface->fd_size(); i != end; ++i)
        close(surface->fd(i));
}

MirSurfaceParameters MirSurface::get_parameters() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return buffer_stream->get_parameters();
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

void MirSurface::acquired_persistent_id(mir_surface_id_callback callback, void* context)
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

MirWaitHandle* MirSurface::request_persistent_id(mir_surface_id_callback callback, void* context)
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
        server->request_persistent_surface_id(0, &surface->id(), persistent_id.get(), gp::NewCallback(this, &MirSurface::acquired_persistent_id, callback, context));
    }
    catch (std::exception const& ex)
    {
        surface->set_error(std::string{"Failed to acquire a persistent ID from the server: "} +
                          boost::diagnostic_information(ex));
    }
    return &persistent_id_wait_handle;
}

MirWaitHandle* MirSurface::get_create_wait_handle()
{
    return &create_wait_handle;
}

/* todo: all these conversion functions are a bit of a kludge, probably
         better to have a more developed MirPixelFormat that can handle this */
MirPixelFormat MirSurface::convert_ipc_pf_to_geometry(gp::int32 pf) const
{
    return static_cast<MirPixelFormat>(pf);
}

void MirSurface::created(mir_surface_callback callback, void * context)
{
    {
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (!surface->has_id())
    {
        if (!surface->has_error())
            surface->set_error("Error processing surface create response, no ID (disconnected?)");

        callback(this, context);
        create_wait_handle.result_received();
        return;
    }
    }
    try
    {
        {
            std::lock_guard<decltype(mutex)> lock(mutex);

            buffer_stream = buffer_stream_factory->
                make_producer_stream(connection, *server, surface->buffer_stream(), name);

            for(int i = 0; i < surface->attributes_size(); i++)
            {
                auto const& attrib = surface->attributes(i);
                attrib_cache[attrib.attrib()] = attrib.ivalue();
            }
        }

        connection->on_surface_created(id(), this);
    }
    catch (std::exception const& error)
    {
        surface->set_error(std::string{"Error processing Surface creating response:"} +
                          boost::diagnostic_information(error));
    }

    callback(this, context);
    create_wait_handle.result_received();
}

MirWaitHandle* MirSurface::release_surface(
        mir_surface_callback callback,
        void * context)
{
    bool was_valid = false;
    {
        std::lock_guard<decltype(handle_mutex)> lock(handle_mutex);
        if (valid_surfaces.count(this))
            was_valid = true;
        valid_surfaces.erase(this);
    }
    if (this->surface->has_error())
        was_valid = false;

    MirWaitHandle* wait_handle{nullptr};
    if (connection && was_valid)
    {
        wait_handle = connection->release_surface(this, callback, context);
    }
    else
    {
        callback(this, context);
        delete this;
    }

    return wait_handle;
}

MirWaitHandle* MirSurface::configure_cursor(MirCursorConfiguration const* cursor)
{
    auto setting = mcl::make_protobuf_object<mp::CursorSetting>();

    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        setting->mutable_surfaceid()->CopyFrom(surface->id());
        if (cursor)
        {
            if (cursor->stream != nullptr)
            {
                setting->mutable_buffer_stream()->set_value(cursor->stream->rpc_id().as_value());
                setting->set_hotspot_x(cursor->hotspot_x);
                setting->set_hotspot_y(cursor->hotspot_y);
            }
            else if (cursor->name != mir_disabled_cursor_name)
            {
                setting->set_name(cursor->name.c_str());
            }
        }
    }
    
    configure_cursor_wait_handle.expect_result();
    server->configure_cursor(0, setting.get(), void_response.get(),
        google::protobuf::NewCallback(this, &MirSurface::on_cursor_configured));
    
    return &configure_cursor_wait_handle;
}

MirWaitHandle* MirSurface::configure(MirSurfaceAttrib at, int value)
{
    // TODO: This is obviously strange. It should be
    // possible to eliminate it in the second phase of buffer
    // stream where the existing MirSurface swap interval functions
    // may be deprecated in terms of mir_buffer_stream_ alternatives
    if (at == mir_surface_attrib_swapinterval)
    {
        buffer_stream->set_swap_interval(value);
        return &configure_wait_handle;
    }

    std::unique_lock<decltype(mutex)> lock(mutex);

    auto setting = mcl::make_protobuf_object<mp::SurfaceSetting>();
    setting->mutable_surfaceid()->CopyFrom(surface->id());
    setting->set_attrib(at);
    setting->set_ivalue(value);
    lock.unlock();

    configure_wait_handle.expect_result();
    server->configure_surface(0, setting.get(), configure_result.get(),
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

    auto request = mcl::make_protobuf_object<mp::CoordinateTranslationRequest>();

    request->set_x(x);
    request->set_y(y);
    *request->mutable_surfaceid() = surface->id();
    auto response = mcl::make_protobuf_object<mp::CoordinateTranslationResponse>();

    MirWaitHandle signal;
    signal.expect_result();

    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        debug->translate_surface_to_screen(
            nullptr,
            request.get(),
            response.get(),
            google::protobuf::NewCallback(&signal_response_received, &signal));
    }

    signal.wait_for_one();

    *screen_x = response->x();
    *screen_y = response->y();
    return !response->has_error();
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
        case mir_surface_attrib_type:
        case mir_surface_attrib_state:
        case mir_surface_attrib_focus:
        case mir_surface_attrib_dpi:
        case mir_surface_attrib_preferred_orientation:
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


int MirSurface::attrib(MirSurfaceAttrib at) const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (at == mir_surface_attrib_swapinterval)
    {
        if (buffer_stream)
            return buffer_stream->swap_interval();
        else // Surface creation is not finalized
            return 1;
    }

    return attrib_cache[at];
}

void MirSurface::set_event_handler(mir_surface_event_callback callback,
                                   void* context)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    input_thread.reset();

    if (callback)
    {
        handle_event_callback = std::bind(callback, this,
                                          std::placeholders::_1,
                                          context);

        if (surface->fd_size() > 0 && handle_event_callback)
        {
            auto input_dispatcher = input_platform->create_input_receiver(surface->fd(0),
                                                                          keymapper,
                                                                          handle_event_callback);
            input_thread = std::make_shared<md::ThreadedDispatcher>("Input dispatch", input_dispatcher);
        }
    }
}

void MirSurface::handle_event(MirEvent const& e)
{
    std::unique_lock<decltype(mutex)> lock(mutex);

    switch (mir_event_get_type(&e))
    {
    case mir_event_type_surface:
    {
        auto sev = mir_event_get_surface_event(&e);
        auto a = mir_surface_event_get_attribute(sev);
        if (a < mir_surface_attribs)
            attrib_cache[a] = mir_surface_event_get_attribute_value(sev);
        break;
    }
    case mir_event_type_orientation:
        orientation = mir_orientation_event_get_direction(mir_event_get_orientation_event(&e));
        break;
    case mir_event_type_keymap:
    {
        xkb_rule_names names;
        mir_keymap_event_get_rules(mir_event_get_keymap_event(&e), &names);
        keymapper->set_rules(names);
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

void MirSurface::request_and_wait_for_configure(MirSurfaceAttrib a, int value)
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
    return configure(mir_surface_attrib_preferred_orientation, mode);
}

mir::client::ClientBufferStream* MirSurface::get_buffer_stream()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    
    return buffer_stream.get();
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

MirWaitHandle* MirSurface::modify(MirSurfaceSpec const& spec)
{
    auto mods = mcl::make_protobuf_object<mp::SurfaceModifications>();

    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        mods->mutable_surface_id()->set_value(surface->id().value());
    }

    auto const surface_specification = mods->mutable_surface_specification();

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
    COPY_IF_SET(min_width);
    COPY_IF_SET(min_height);
    COPY_IF_SET(max_width);
    COPY_IF_SET(max_height);
    COPY_IF_SET(width_inc);
    COPY_IF_SET(height_inc);
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
        for(auto const& stream : spec.streams.value())
        {
            auto const new_stream = surface_specification->add_stream();
            new_stream->set_displacement_x(stream.displacement_x);
            new_stream->set_displacement_y(stream.displacement_y);
            new_stream->mutable_id()->set_value(
                reinterpret_cast<mcl::ClientBufferStream*>(stream.stream)->rpc_id().as_value());
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

    modify_wait_handle.expect_result();
    server->modify_surface(0, mods.get(), modify_result.get(),
              google::protobuf::NewCallback(this, &MirSurface::on_modified));

    return &modify_wait_handle;
}
