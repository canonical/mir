/*
 * Copyright © 2014-2016 Canonical Ltd.
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

#define MIR_LOG_COMPONENT "MirWindowAPI"

#include "mir_toolkit/mir_surface.h"
#include "mir_toolkit/mir_wait.h"
#include "mir_toolkit/mir_presentation_chain.h"
#include "mir/require.h"

#include "mir_connection.h"
#include "mir_surface.h"
#include "presentation_chain.h"
#include "render_surface.h"
#include "error_connections.h"
#include "connection_surface_map.h"
#include "mir/uncaught.h"

#include <boost/exception/diagnostic_information.hpp>
#include <functional>
#include <algorithm>

namespace mcl = mir::client;

namespace
{

// assign_result is compatible with all 2-parameter callbacks
void assign_result(void* result, void** context)
{
    if (context)
        *context = result;
}

}

MirWindowSpec*
mir_specify_window(MirConnection* connection,
                   int width, int height,
                   MirPixelFormat format)
{
    auto spec = new MirWindowSpec{connection, width, height, format};
    spec->type = mir_surface_type_normal;
    return spec;
}

MirWindowSpec*
mir_specify_menu(MirConnection* connection,
                int width, int height,
                MirPixelFormat format,
                MirSurface* parent,
                MirRectangle* rect,
                MirEdgeAttachment edge)
{
    mir::require(mir_surface_is_valid(parent));
    mir::require(rect != nullptr);

    auto spec = new MirWindowSpec{connection, width, height, format};
    spec->type = mir_surface_type_menu;
    spec->parent = parent;
    spec->aux_rect = *rect;
    spec->edge_attachment = edge;
    return spec;
}

MirWindowSpec* mir_specify_tip(MirConnection* connection,
                               int width, int height,
                               MirPixelFormat format,
                               MirSurface* parent,
                               MirRectangle* rect,
                               MirEdgeAttachment edge)
{
    mir::require(mir_surface_is_valid(parent));
    mir::require(rect != nullptr);

    auto spec = new MirWindowSpec{connection, width, height, format};
    spec->type = mir_surface_type_tip;
    spec->parent = parent;
    spec->aux_rect = *rect;
    spec->edge_attachment = edge;
    return spec;
}

MirWindowSpec* mir_specify_modal_dialog(MirConnection* connection,
                                        int width, int height,
                                        MirPixelFormat format,
                                        MirSurface* parent)
{
    mir::require(mir_surface_is_valid(parent));

    auto spec = new MirWindowSpec{connection, width, height, format};
    spec->type = mir_surface_type_dialog;
    spec->parent = parent;

    return spec;
}

MirWindowSpec*
mir_specify_dialog(MirConnection* connection,
                   int width, int height,
                   MirPixelFormat format)
{
    auto spec = new MirWindowSpec{connection, width, height, format};
    spec->type = mir_surface_type_dialog;
    return spec;
}

MirWindowSpec*
mir_specify_input_method(MirConnection* connection,
                            int width, int height,
                            MirPixelFormat format)
{
    auto spec = new MirWindowSpec{connection, width, height, format};
    spec->type = mir_surface_type_inputmethod;
    return spec;
}

MirWindowSpec* mir_create_spec(MirConnection* connection)
try
{
    mir::require(mir_connection_is_valid(connection));
    auto const spec = new MirWindowSpec{};
    spec->connection = connection;
    return spec;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    std::abort();  // If we just failed to allocate a MirWindowSpec returning isn't safe
}

void mir_spec_set_parent(MirWindowSpec* spec, MirSurface* parent)
try
{
    mir::require(spec);
    spec->parent = parent;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_type(MirWindowSpec* spec, MirSurfaceType type)
try
{
    mir::require(spec);
    spec->type = type;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_name(MirWindowSpec* spec, char const* name)
try
{
    mir::require(spec);
    spec->surface_name = name;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_width(MirWindowSpec* spec, unsigned width)
try
{
    mir::require(spec);
    spec->width = width;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_height(MirWindowSpec* spec, unsigned height)
try
{
    mir::require(spec);
    spec->height = height;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_width_increment(MirWindowSpec* spec, unsigned width_inc)
try
{
    mir::require(spec);
    spec->width_inc = width_inc;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_height_increment(MirWindowSpec* spec, unsigned height_inc)
try
{
    mir::require(spec);
    spec->height_inc = height_inc;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_min_width(MirWindowSpec* spec, unsigned min_width)
try
{
    mir::require(spec);
    spec->min_width = min_width;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_min_height(MirWindowSpec* spec, unsigned min_height)
try
{
    mir::require(spec);
    spec->min_height = min_height;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_max_width(MirWindowSpec* spec, unsigned max_width)
try
{
    mir::require(spec);
    spec->max_width = max_width;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_max_height(MirWindowSpec* spec, unsigned max_height)
try
{
    mir::require(spec);
    spec->max_height = max_height;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_min_aspect_ratio(MirWindowSpec* spec, unsigned width, unsigned height)
try
{
    mir::require(spec);
    spec->min_aspect = {width, height};
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_max_aspect_ratio(MirWindowSpec* spec, unsigned width, unsigned height)
try
{
    mir::require(spec);
    spec->max_aspect = {width, height};
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_fullscreen(MirWindowSpec* spec, uint32_t output_id)
try
{
    mir::require(spec);
    spec->output_id = output_id;
    spec->state = mir_surface_state_fullscreen;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_preferred_orientation(MirWindowSpec* spec, MirOrientationMode mode)
try
{
    mir::require(spec);
    spec->pref_orientation = mode;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

bool mir_spec_attach_to_foreign_parent(MirWindowSpec* spec,
                                               MirPersistentId* parent,
                                               MirRectangle* attachment_rect,
                                               MirEdgeAttachment edge)
{
    mir::require(mir_persistent_id_is_valid(parent));
    mir::require(attachment_rect != nullptr);

    if (!spec->type.is_set() ||
        spec->type.value() != mir_surface_type_inputmethod)
    {
        return false;
    }

    spec->parent_id = std::make_unique<MirPersistentId>(*parent);
    spec->aux_rect = *attachment_rect;
    spec->edge_attachment = edge;
    return true;
}

void mir_spec_set_state(MirWindowSpec* spec, MirSurfaceState state)
try
{
    mir::require(spec);
    spec->state = state;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_input_shape(MirWindowSpec *spec,
                              MirRectangle const* rectangles,
                              size_t n_rects)
try
{
    mir::require(spec);

    std::vector<MirRectangle> copy;
    for (auto i = 0u; i < n_rects; i++)
    {
        copy.emplace_back(rectangles[i]);
    }
    spec->input_shape = copy;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_event_handler(MirWindowSpec* spec,
                                mir_surface_event_callback callback,
                                void* context)
try
{
    mir::require(spec);
    spec->event_handler = MirWindowSpec::EventHandler{callback, context};
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_shell_chrome(MirWindowSpec* spec, MirShellChrome style)
try
{
    mir::require(spec);
    spec->shell_chrome = style;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_pointer_confinement(MirWindowSpec* spec, MirPointerConfinementState state)
try
{
    mir::require(spec);
    spec->confine_pointer = state;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_placement(MirWindowSpec* spec,
                            MirRectangle const* rect,
                            MirPlacementGravity rect_gravity,
                            MirPlacementGravity window_gravity,
                            MirPlacementHints placement_hints,
                            int offset_dx, int offset_dy)
try
{
    mir::require(spec);
    spec->aux_rect = *rect;
    spec->aux_rect_placement_gravity = rect_gravity;
    spec->surface_placement_gravity = window_gravity;
    spec->placement_hints = placement_hints;
    spec->aux_rect_placement_offset_x = offset_dx;
    spec->aux_rect_placement_offset_y = offset_dy;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_set_cursor_name(MirWindowSpec* spec, char const* name)
try
{
    mir::require(spec);
    spec->cursor_name = std::string(name);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_spec_release(MirWindowSpec* spec)
{
    delete spec;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

MirSurfaceSpec* mir_connection_create_spec_for_normal_surface(MirConnection* connection,
                                                              int width, int height,
                                                              MirPixelFormat format)
{
    return mir_specify_window(connection, width, height, format);
}

MirSurfaceSpec* mir_connection_create_spec_for_menu(MirConnection* connection,
                                                    int width, int height,
                                                    MirPixelFormat format,
                                                    MirSurface* parent,
                                                    MirRectangle* rect,
                                                    MirEdgeAttachment edge)
{
    return mir_specify_menu(connection, width, height, format, parent, rect, edge);
}

MirSurfaceSpec* mir_connection_create_spec_for_tooltip(MirConnection* connection,
                                                       int width, int height,
                                                       MirPixelFormat format,
                                                       MirSurface* parent,
                                                       MirRectangle* rect)
{
    return mir_specify_tip(connection, width, height, format, parent, rect, mir_edge_attachment_any);
}

MirSurfaceSpec* mir_connection_create_spec_for_tip(MirConnection* connection,
                                                   int width, int height,
                                                   MirPixelFormat format,
                                                   MirSurface* parent,
                                                   MirRectangle* rect,
                                                   MirEdgeAttachment edge)
{
    return mir_specify_tip(connection, width, height, format, parent, rect, edge);
}

MirSurfaceSpec* mir_connection_create_spec_for_dialog(MirConnection* connection,
                                                      int width,
                                                      int height,
                                                      MirPixelFormat format)
{
    return mir_specify_dialog(connection, width, height, format);
}

MirSurfaceSpec* mir_connection_create_spec_for_input_method(MirConnection* connection,
                                                            int width, int height,
                                                            MirPixelFormat format)
{
    return mir_specify_input_method(connection, width, height, format);
}

MirSurfaceSpec* mir_connection_create_spec_for_modal_dialog(MirConnection* connection,
                                                           int width, int height,
                                                           MirPixelFormat format,
                                                           MirSurface* parent)
{
    return mir_specify_modal_dialog(connection, width, height, format, parent);
}

MirSurface* mir_surface_create_sync(MirSurfaceSpec* requested_specification)
{
    MirSurface* surface = nullptr;

    mir_wait_for(mir_surface_create(requested_specification,
        reinterpret_cast<mir_surface_callback>(assign_result),
        &surface));

    return surface;
}

MirWaitHandle* mir_surface_create(MirSurfaceSpec* requested_specification,
                                  mir_surface_callback callback, void* context)
{
    mir::require(requested_specification != nullptr);
    mir::require(requested_specification->type.is_set());

    auto conn = requested_specification->connection;
    mir::require(mir_connection_is_valid(conn));

    try
    {
        return conn->create_surface(*requested_specification, callback, context);
    }
    catch (std::exception const& error)
    {
        return nullptr;
    }
}

void mir_surface_spec_set_name(MirSurfaceSpec* spec, char const* name)
{
    mir_surface_spec_set_name(spec, name);
}

void mir_surface_spec_set_width(MirSurfaceSpec* spec, unsigned width)
{
    mir_spec_set_width(spec, width);
}

void mir_surface_spec_set_height(MirSurfaceSpec* spec, unsigned height)
{
    mir_spec_set_height(spec, height);
}

void mir_surface_spec_set_min_width(MirSurfaceSpec* spec, unsigned min_width)
{
    mir_spec_set_min_width(spec, min_width);
}

void mir_surface_spec_set_min_height(MirSurfaceSpec* spec, unsigned min_height)
{
    mir_spec_set_min_height(spec, min_height);
}

void mir_surface_spec_set_max_width(MirSurfaceSpec* spec, unsigned max_width)
{
    mir_spec_set_max_width(spec, max_width);
}

void mir_surface_spec_set_max_height(MirSurfaceSpec* spec, unsigned max_height)
{
    mir_spec_set_max_height(spec, max_height);
}

void mir_surface_spec_set_pixel_format(MirSurfaceSpec* spec, MirPixelFormat format)
{
    spec->pixel_format = format;
}

void mir_surface_spec_set_buffer_usage(MirSurfaceSpec* spec, MirBufferUsage usage)
{
    spec->buffer_usage = usage;
}

void mir_surface_spec_set_state(MirSurfaceSpec* spec, MirSurfaceState state)
{
    mir_spec_set_state(spec, state);
}

void mir_surface_spec_set_fullscreen_on_output(MirSurfaceSpec* spec, uint32_t output_id)
{
    mir_spec_set_fullscreen(spec, output_id);
}

void mir_surface_spec_set_preferred_orientation(MirSurfaceSpec* spec, MirOrientationMode mode)
{
    mir_spec_set_preferred_orientation(spec, mode);
}

void mir_surface_spec_set_event_handler(MirSurfaceSpec* spec,
                                        mir_surface_event_callback callback,
                                        void* context)
{
    mir_spec_set_event_handler(spec, callback, context);
}

void mir_surface_spec_set_shell_chrome(MirSurfaceSpec* spec, MirShellChrome style)
{
    mir_spec_set_shell_chrome(spec, style);
}

void mir_surface_spec_release(MirSurfaceSpec* spec)
{
    mir_spec_release(spec);
}

MirSurfaceSpec* mir_create_surface_spec(MirConnection* connection)
{
    return mir_create_spec(connection);
}

MirSurfaceSpec* mir_connection_create_spec_for_changes(MirConnection* connection)
{
    return mir_create_spec(connection);
}

void mir_surface_apply_spec(MirSurface* surface, MirSurfaceSpec* spec)
try
{
    mir::require(mir_surface_is_valid(surface));
    mir::require(spec);

    surface->modify(*spec);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    // Keep calm and carry on
}

void mir_surface_spec_set_streams(MirSurfaceSpec* spec, MirBufferStreamInfo* streams, unsigned int size)
try
{
    mir::require(spec);

    std::vector<ContentInfo> copy;
    for (auto i = 0u; i < size; i++)
    {
        mir::require(mir_buffer_stream_is_valid(streams[i].stream));
        copy.emplace_back(ContentInfo{
            mir::geometry::Displacement{streams[i].displacement_x, streams[i].displacement_y},
            streams[i].stream->rpc_id().as_value(),
            {}});
    }
    spec->streams = copy;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_surface_spec_add_presentation_chain(
    MirSurfaceSpec* spec,
    int width, int height,
    int displacement_x, int displacement_y,
    MirPresentationChain* client_chain)
try
{
    mir::require(spec && client_chain);
    auto chain = reinterpret_cast<mcl::PresentationChain*>(client_chain);

    ContentInfo info{
        {displacement_x, displacement_y}, chain->rpc_id(), mir::geometry::Size{width, height}};
    if (spec->streams.is_set())
        spec->streams.value().push_back(info);
    else
        spec->streams = std::vector<ContentInfo>{info};
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_surface_spec_add_buffer_stream(
    MirSurfaceSpec* spec,
    int displacement_x, int displacement_y,
    int width, int height,
    MirBufferStream* stream)
try
{
    mir::require(spec && stream);
    ContentInfo info{{displacement_x, displacement_y}, stream->rpc_id().as_value(), mir::geometry::Size{width, height}};

    if (spec->streams.is_set())
        spec->streams.value().push_back(info);
    else
        spec->streams = std::vector<ContentInfo>{info};
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_surface_spec_add_render_surface(
    MirSurfaceSpec* spec,
    MirRenderSurface* render_surface,
    int /*logical_width*/, int /*logical_height*/,
    int displacement_x, int displacement_y)
try
{
    mir::require(spec && render_surface);
    auto rs = spec->connection->connection_surface_map()->render_surface(render_surface);

    if (rs->stream_id().as_value() < 0)
        BOOST_THROW_EXCEPTION(std::logic_error("Render surface holds no content."));
    ContentInfo info{{displacement_x, displacement_y}, rs->stream_id().as_value(),{}};

    if (spec->streams.is_set())
        spec->streams.value().push_back(info);
    else
        spec->streams = std::vector<ContentInfo>{info};
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_surface_spec_set_input_shape(MirSurfaceSpec *spec, MirRectangle const* rectangles,
                                      size_t n_rects)
{
    mir_spec_set_input_shape(spec, rectangles, n_rects);
}

void mir_surface_spec_set_parent(MirSurfaceSpec* spec, MirSurface* parent)
{
    mir_spec_set_parent(spec, parent);
}

void mir_surface_spec_set_type(MirSurfaceSpec* spec, MirSurfaceType type)
{
    mir_spec_set_type(spec, type);
}

void mir_surface_spec_set_width_increment(MirSurfaceSpec* spec, unsigned width_inc)
{
    mir_spec_set_width_increment(spec, width_inc);
}

void mir_surface_spec_set_height_increment(MirSurfaceSpec* spec, unsigned height_inc)
{
    mir_spec_set_height_increment(spec, height_inc);
}

void mir_surface_spec_set_min_aspect_ratio(MirSurfaceSpec* spec, unsigned width, unsigned height)
{
    mir_spec_set_min_aspect_ratio(spec, width, height);
}

void mir_surface_spec_set_max_aspect_ratio(MirSurfaceSpec* spec, unsigned width, unsigned height)
{
    mir_spec_set_max_aspect_ratio(spec, width, height);
}

void mir_surface_spec_set_pointer_confinement(MirSurfaceSpec* spec, MirPointerConfinementState state)
{
    mir_spec_set_pointer_confinement(spec, state);
}

void mir_surface_spec_set_placement(MirSurfaceSpec* spec,
                                    MirRectangle const* rect,
                                    MirPlacementGravity rect_gravity,
                                    MirPlacementGravity window_gravity,
                                    MirPlacementHints placement_hints,
                                    int offset_dx, int offset_dy)
{
    mir_spec_set_placement(spec,
                           rect,
                           rect_gravity,
                           window_gravity,
                           placement_hints,
                           offset_dx,
                           offset_dy);
}

bool mir_surface_spec_attach_to_foreign_parent(MirSurfaceSpec* spec,
                                               MirPersistentId* parent,
                                               MirRectangle* attachment_rect,
                                               MirEdgeAttachment edge)
{
    return mir_spec_attach_to_foreign_parent(spec,
                                             parent,
                                             attachment_rect,
                                             edge);
}

#pragma GCC diagnostic pop

extern "C"
void mir_surface_set_event_handler(MirSurface* surface, mir_surface_event_callback callback, void* context)
{
    surface->set_event_handler(callback, context);
}


bool mir_surface_is_valid(MirSurface* surface)
{
    return MirSurface::is_valid(surface);
}

char const* mir_surface_get_error_message(MirSurface* surface)
{
    return surface->get_error_message();
}

void mir_surface_get_parameters(MirSurface* surface, MirSurfaceParameters* parameters)
{
    *parameters = surface->get_parameters();
}

MirWaitHandle* mir_surface_release(
    MirSurface* surface,
    mir_surface_callback callback, void* context)
{
    auto connection = surface->connection();
    try
    {
        return connection->release_surface(surface, callback, context);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }
}

void mir_surface_release_sync(MirSurface* surface)
{
    mir_wait_for(mir_surface_release(surface,
        reinterpret_cast<mir_surface_callback>(assign_result),
        nullptr));
}

MirSurfaceType mir_surface_get_type(MirSurface* surf)
{
    MirSurfaceType type = mir_surface_type_normal;

    if (surf)
    {
        // Only the client will ever change the type of a surface so it is
        // safe to get the type from a local cache surf->attrib().

        int t = surf->attrib(mir_surface_attrib_type);
        type = static_cast<MirSurfaceType>(t);
    }

    return type;
}

MirWaitHandle* mir_surface_set_state(MirSurface* surf, MirSurfaceState state)
{
    try
    {
        return surf ? surf->configure(mir_surface_attrib_state, state) : nullptr;
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }
}

MirSurfaceState mir_surface_get_state(MirSurface* surf)
{
    MirSurfaceState state = mir_surface_state_unknown;

    try
    {
        if (surf)
        {
            int s = surf->attrib(mir_surface_attrib_state);

            if (s == mir_surface_state_unknown)
            {
                surf->configure(mir_surface_attrib_state,
                                mir_surface_state_unknown)->wait_for_all();
                s = surf->attrib(mir_surface_attrib_state);
            }

            state = static_cast<MirSurfaceState>(s);
        }
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return state;
}

MirOrientation mir_surface_get_orientation(MirSurface *surface)
{
    return surface->get_orientation();
}

MirWaitHandle* mir_surface_set_swapinterval(MirSurface* surf, int interval)
{
    if ((interval < 0) || (interval > 1))
        return nullptr;

    try
    {
        if (surf)
        {
            if (auto stream = surf->get_buffer_stream())
                return stream->set_swap_interval(interval);
        }
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }
    return nullptr;
}

int mir_surface_get_swapinterval(MirSurface* surf)
{
    int swap_interval = -1;

    try
    {
        if (surf)
        {
            if (auto stream = surf->get_buffer_stream())
                swap_interval = stream->swap_interval();
        }
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return swap_interval;
}

int mir_surface_get_dpi(MirSurface* surf)
{
    int dpi = -1;

    try
    {
        if (surf)
        {
            dpi = surf->attrib(mir_surface_attrib_dpi);
        }
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return dpi;
}

MirSurfaceFocusState mir_surface_get_focus(MirSurface* surf)
{
    MirSurfaceFocusState state = mir_surface_unfocused;

    try
    {
        if (surf)
        {
            state = static_cast<MirSurfaceFocusState>(surf->attrib(mir_surface_attrib_focus));
        }
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return state;
}

MirSurfaceVisibility mir_surface_get_visibility(MirSurface* surf)
{
    MirSurfaceVisibility state = static_cast<MirSurfaceVisibility>(mir_surface_visibility_occluded);

    try
    {
        if (surf)
        {
            state = static_cast<MirSurfaceVisibility>(surf->attrib(mir_surface_attrib_visibility));
        }
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return state;
}

MirWaitHandle* mir_surface_configure_cursor(MirSurface* surface, MirCursorConfiguration const* cursor)
{
    MirWaitHandle *result = nullptr;
    
    try
    {
        if (surface)
            result = surface->configure_cursor(cursor);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return result;
}

MirOrientationMode mir_surface_get_preferred_orientation(MirSurface *surf)
{
    mir::require(mir_surface_is_valid(surf));

    MirOrientationMode mode = mir_orientation_mode_any;

    try
    {
        mode = static_cast<MirOrientationMode>(surf->attrib(mir_surface_attrib_preferred_orientation));
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return mode;
}

MirWaitHandle* mir_surface_set_preferred_orientation(MirSurface *surf, MirOrientationMode mode)
{
    mir::require(mir_surface_is_valid(surf));

    MirWaitHandle *result{nullptr};
    try
    {
        result = surf->set_preferred_orientation(mode);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return result;
}

void mir_surface_raise(MirSurface* surf, MirCookie const* cookie)
{
    mir::require(mir_surface_is_valid(surf));

    try
    {
        surf->raise_surface(cookie);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }
}

MirBufferStream *mir_surface_get_buffer_stream(MirSurface *surface)
try
{
    return surface->get_buffer_stream();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirWaitHandle* mir_surface_request_persistent_id(MirSurface* surface, mir_surface_id_callback callback, void* context)
{
    mir::require(mir_surface_is_valid(surface));

    return surface->request_persistent_id(callback, context);
}

namespace
{
void assign_surface_id_result(MirSurface*, MirPersistentId* id, void* context)
{
    void** result_ptr = reinterpret_cast<void**>(context);
    *result_ptr = id;
}
}

MirPersistentId* mir_surface_request_persistent_id_sync(MirSurface *surface)
{
    mir::require(mir_surface_is_valid(surface));

    MirPersistentId* result = nullptr;
    mir_wait_for(mir_surface_request_persistent_id(surface,
                                                   &assign_surface_id_result,
                                                   &result));
    return result;
}

bool mir_persistent_id_is_valid(MirPersistentId* id)
{
    return id != nullptr;
}

void mir_persistent_id_release(MirPersistentId* id)
{
    delete id;
}

char const* mir_persistent_id_as_string(MirPersistentId *id)
{
    return id->as_string().c_str();
}

MirPersistentId* mir_persistent_id_from_string(char const* id_string)
{
    return new MirPersistentId{id_string};
}
