/*
 * Copyright © 2014-2016 Canonical Ltd.
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirWaitHandle* mir_configure_cursor_helper(MirWindow* window, MirCursorConfiguration const* cursor)
{
    MirWaitHandle *result = nullptr;

    try
    {
        if (window)
            result = window->configure_cursor(cursor);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return result;
}
}
#pragma GCC diagnostic pop

MirWindowSpec*
mir_create_normal_window_spec(MirConnection* connection,
                              int width, int height)
{
    auto spec = new MirWindowSpec{connection, width, height, mir_pixel_format_invalid};
    spec->type = mir_window_type_normal;
    return spec;
}

MirWindowSpec*
mir_create_menu_window_spec(MirConnection* connection,
                            int width, int height,
                            MirWindow* parent,
                            MirRectangle* rect,
                            MirEdgeAttachment edge)
{
    mir::require(mir_window_is_valid(parent));
    mir::require(rect != nullptr);

    auto spec = new MirWindowSpec{connection, width, height, mir_pixel_format_invalid};
    spec->type = mir_window_type_menu;
    spec->parent = parent;
    spec->aux_rect = *rect;
    spec->edge_attachment = edge;
    return spec;
}

MirWindowSpec* mir_create_tip_window_spec(MirConnection* connection,
                                          int width, int height,
                                          MirWindow* parent,
                                          MirRectangle* rect,
                                          MirEdgeAttachment edge)
{
    mir::require(mir_window_is_valid(parent));
    mir::require(rect != nullptr);

    auto spec = new MirWindowSpec{connection, width, height, mir_pixel_format_invalid};
    spec->type = mir_window_type_tip;
    spec->parent = parent;
    spec->aux_rect = *rect;
    spec->edge_attachment = edge;
    return spec;
}

MirWindowSpec* mir_create_modal_dialog_window_spec(MirConnection* connection,
                                                   int width, int height,
                                                   MirWindow* parent)
{
    mir::require(mir_window_is_valid(parent));

    auto spec = new MirWindowSpec{connection, width, height, mir_pixel_format_invalid};
    spec->type = mir_window_type_dialog;
    spec->parent = parent;

    return spec;
}

MirWindowSpec*
mir_create_dialog_window_spec(MirConnection* connection,
                              int width, int height)
{
    auto spec = new MirWindowSpec{connection, width, height, mir_pixel_format_invalid};
    spec->type = mir_window_type_dialog;
    return spec;
}

MirWindowSpec*
mir_create_input_method_window_spec(MirConnection* connection,
                                    int width, int height)
{
    auto spec = new MirWindowSpec{connection, width, height, mir_pixel_format_invalid};
    spec->type = mir_window_type_inputmethod;
    return spec;
}

MirWindowSpec*
mir_create_gloss_window_spec(MirConnection* connection, int width, int height)
{
    auto spec = new MirWindowSpec{connection, width, height, mir_pixel_format_invalid};
    spec->type = mir_window_type_gloss;
    return spec;
}

MirWindowSpec*
mir_create_satellite_window_spec(MirConnection* connection, int width, int height, MirWindow* parent)
{
    mir::require(mir_window_is_valid(parent));

    auto spec = new MirWindowSpec{connection, width, height, mir_pixel_format_invalid};
    spec->type = mir_window_type_satellite;
    spec->parent = parent;

    return spec;
}

MirWindowSpec*
mir_create_utility_window_spec(MirConnection* connection, int width, int height)
{
    auto spec = new MirWindowSpec{connection, width, height, mir_pixel_format_invalid};
    spec->type = mir_window_type_utility;
    return spec;
}

MirWindowSpec*
mir_create_freestyle_window_spec(MirConnection* connection, int width, int height){
    auto spec = new MirWindowSpec{connection, width, height, mir_pixel_format_invalid};
    spec->type = mir_window_type_freestyle;
    return spec;
}

MirWindowSpec* mir_create_window_spec(MirConnection* connection)
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

MirConnection* mir_window_get_connection(MirWindow* window)
try
{
    mir::require(mir_window_is_valid(window));

    return window->connection();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    std::abort();  // If we just failed to allocate a MirWindowSpec returning isn't safe
}

void mir_window_spec_set_parent(MirWindowSpec* spec, MirWindow* parent)
try
{
    mir::require(spec);
    spec->parent = parent;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_type(MirWindowSpec* spec, MirWindowType type)
try
{
    mir::require(spec);
    spec->type = type;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_name(MirWindowSpec* spec, char const* name)
try
{
    mir::require(spec);
    spec->surface_name = name;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_width(MirWindowSpec* spec, unsigned width)
try
{
    mir::require(spec);
    spec->width = width;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_height(MirWindowSpec* spec, unsigned height)
try
{
    mir::require(spec);
    spec->height = height;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_width_increment(MirWindowSpec* spec, unsigned width_inc)
try
{
    mir::require(spec);
    spec->width_inc = width_inc;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_height_increment(MirWindowSpec* spec, unsigned height_inc)
try
{
    mir::require(spec);
    spec->height_inc = height_inc;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_min_width(MirWindowSpec* spec, unsigned min_width)
try
{
    mir::require(spec);
    spec->min_width = min_width;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_min_height(MirWindowSpec* spec, unsigned min_height)
try
{
    mir::require(spec);
    spec->min_height = min_height;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_max_width(MirWindowSpec* spec, unsigned max_width)
try
{
    mir::require(spec);
    spec->max_width = max_width;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_max_height(MirWindowSpec* spec, unsigned max_height)
try
{
    mir::require(spec);
    spec->max_height = max_height;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_min_aspect_ratio(MirWindowSpec* spec, unsigned width, unsigned height)
try
{
    mir::require(spec);
    spec->min_aspect = {width, height};
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_max_aspect_ratio(MirWindowSpec* spec, unsigned width, unsigned height)
try
{
    mir::require(spec);
    spec->max_aspect = {width, height};
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_fullscreen_on_output(MirWindowSpec* spec, uint32_t output_id)
try
{
    mir::require(spec);
    spec->output_id = output_id;
    spec->state = mir_window_state_fullscreen;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_preferred_orientation(MirWindowSpec* spec, MirOrientationMode mode)
try
{
    mir::require(spec);
    spec->pref_orientation = mode;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

bool mir_window_spec_attach_to_foreign_parent(MirWindowSpec* spec,
                                              MirWindowId* parent,
                                              MirRectangle* attachment_rect,
                                              MirEdgeAttachment edge)
{
    mir::require(mir_window_id_is_valid(parent));
    mir::require(attachment_rect != nullptr);

    if (!spec->type.is_set() ||
        spec->type.value() != mir_window_type_inputmethod)
    {
        return false;
    }

    spec->parent_id = std::make_unique<MirWindowId>(*parent);
    spec->aux_rect = *attachment_rect;
    spec->edge_attachment = edge;
    return true;
}

void mir_window_spec_set_state(MirWindowSpec* spec, MirWindowState state)
try
{
    mir::require(spec);
    spec->state = state;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_input_shape(MirWindowSpec *spec,
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

void mir_window_spec_set_event_handler(MirWindowSpec* spec,
                                       MirWindowEventCallback callback,
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

void mir_window_spec_set_shell_chrome(MirWindowSpec* spec, MirShellChrome style)
try
{
    mir::require(spec);
    spec->shell_chrome = style;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_pointer_confinement(MirWindowSpec* spec, MirPointerConfinementState state)
try
{
    mir::require(spec);
    spec->confine_pointer = state;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_placement(MirWindowSpec* spec,
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

void mir_window_spec_set_cursor_name(MirWindowSpec* spec, char const* name)
try
{
    mir::require(spec);
    spec->cursor_name = std::string(name);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_window_spec_set_pixel_format(MirWindowSpec* spec, MirPixelFormat format)
try
{
    mir::require(spec);
    spec->pixel_format = format;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void mir_window_spec_set_buffer_usage(MirWindowSpec* spec, MirBufferUsage usage)
try
{
    mir::require(spec);
    spec->buffer_usage = usage;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}
#pragma GCC diagnostic pop

void mir_window_spec_set_streams(MirWindowSpec* spec, MirBufferStreamInfo* streams, unsigned int size)
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

void mir_window_spec_release(MirWindowSpec* spec)
{
    delete spec;
}

void mir_window_apply_spec(MirWindow* window, MirWindowSpec* spec)
try
{
    mir::require(mir_window_is_valid(window));
    mir::require(spec);

    window->modify(*spec);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    // Keep calm and carry on
}

namespace
{
static MirWaitHandle* window_create_helper(MirWindowSpec* requested_specification,
                                           MirWindowCallback callback, void* context)
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

static MirWaitHandle* window_release_helper(
    MirWindow* window,
    MirWindowCallback callback, void* context)
{
    mir::require(window);

    auto connection = window->connection();
    mir::require(mir_connection_is_valid(connection));

    try
    {
        return connection->release_surface(window, callback, context);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }
}

static MirWaitHandle* mir_window_request_persistent_id_helper(
    MirWindow* window, MirWindowIdCallback callback, void* context)
{
    mir::require(mir_window_is_valid(window));

    try
    {
        return window->request_persistent_id(callback, context);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }
}

class WindowSync
{
public:
    void set_result(MirWindow* result)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        window = result;
        cv.notify_all();
    }

    MirWindow* wait_for_result()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        cv.wait(lk, [this] { return window; });
        return window;
    }

private:
    MirWindow* window = nullptr;
    std::mutex mutex;
    std::condition_variable cv;
};

void set_result(MirWindow* result, WindowSync* context)
{
    context->set_result(result);
}
}

void mir_create_window(MirWindowSpec* requested_specification,
                       MirWindowCallback callback, void* context)
{
    window_create_helper(requested_specification, callback, context);
}

MirWindow* mir_create_window_sync(MirWindowSpec* requested_specification)
{
    WindowSync ws;
    mir_create_window(requested_specification,
                      reinterpret_cast<MirWindowCallback>(set_result),
                      &ws);
    return ws.wait_for_result();
}

void mir_window_release(
    MirWindow* window,
    MirWindowCallback callback,
    void *context)
{
    window_release_helper(window, callback, context);
}

void mir_window_release_sync(MirWindow* window)
{
    WindowSync ws;
    mir_window_release(window,
                       reinterpret_cast<MirWindowCallback>(set_result),
                       &ws);
    ws.wait_for_result();
}

bool mir_window_is_valid(MirWindow* window)
{
    return MirWindow::is_valid(window);
}

void mir_window_set_event_handler(MirWindow* window,
                                  MirWindowEventCallback callback,
                                  void* context)
{
    window->set_event_handler(callback, context);
}

MirBufferStream *mir_window_get_buffer_stream(MirWindow* window)
try
{
    return window->get_buffer_stream();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

char const* mir_window_get_error_message(MirWindow* window)
{
    return window->get_error_message();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void mir_window_get_parameters(MirWindow* window, MirWindowParameters* parameters)
{
    *parameters = window->get_parameters();
}
#pragma GCC diagnostic pop

MirOrientation mir_window_get_orientation(MirWindow* window)
{
    return window->get_orientation();
}

void mir_window_raise(MirWindow* window, MirCookie const* cookie)
{
    mir::require(mir_window_is_valid(window));

    try
    {
        window->raise_surface(cookie);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }
}

void mir_window_request_user_move(MirWindow* window, MirCookie const* cookie)
{
    mir::require(mir_window_is_valid(window));

    try
    {
        window->request_user_move(cookie);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }
}

void mir_window_request_user_resize(MirWindow* window, MirCookie const* cookie)
{
    mir::require(mir_window_is_valid(window));

    try
    {
        window->request_user_resize(cookie);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }
}

MirWindowType mir_window_get_type(MirWindow* window)
{
    MirWindowType type = mir_window_type_normal;

    if (window)
    {
        // Only the client will ever change the type of a window so it is
        // safe to get the type from a local cache surf->attrib().

        int t = window->attrib(mir_window_attrib_type);
        type = static_cast<MirWindowType>(t);
    }

    return type;
}

void mir_window_set_state(MirWindow* window, MirWindowState state)
try
{
    mir::require(mir_window_is_valid(window));
        window->configure(mir_window_attrib_state, state);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

MirWindowState mir_window_get_state(MirWindow* window)
{
    MirWindowState state = mir_window_state_unknown;

    try
    {
        if (window)
        {
            int s = window->attrib(mir_window_attrib_state);

            if (s == mir_window_state_unknown)
            {
                window->configure(mir_window_attrib_state,
                    mir_window_state_unknown)->wait_for_all();
                s = window->attrib(mir_window_attrib_state);
            }

            state = static_cast<MirWindowState>(s);
        }
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return state;
}

MirWindowFocusState mir_window_get_focus_state(MirWindow* window)
{
    MirWindowFocusState state = mir_window_focus_state_unfocused;

    try
    {
        if (window)
        {
            state = static_cast<MirWindowFocusState>(window->attrib(mir_window_attrib_focus));
        }
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return state;
}

MirWindowVisibility mir_window_get_visibility(MirWindow* window)
{
    MirWindowVisibility state = static_cast<MirWindowVisibility>(mir_window_visibility_occluded);

    try
    {
        if (window)
        {
            state = static_cast<MirWindowVisibility>(window->attrib(mir_window_attrib_visibility));
        }
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return state;
}

int mir_window_get_dpi(MirWindow* window)
{
    int dpi = -1;

    try
    {
        if (window)
        {
            dpi = window->attrib(mir_window_attrib_dpi);
        }
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return dpi;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void mir_window_configure_cursor(MirWindow* window, MirCursorConfiguration const* cursor)
{
    mir_configure_cursor_helper(window, cursor);
}
#pragma GCC diagnostic pop

void mir_window_set_preferred_orientation(MirWindow* window, MirOrientationMode mode)
try
{
    mir::require(mir_window_is_valid(window));
    window->set_preferred_orientation(mode);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

MirOrientationMode mir_window_get_preferred_orientation(MirWindow* window)
{
    mir::require(mir_window_is_valid(window));

    MirOrientationMode mode = mir_orientation_mode_any;

    try
    {
        mode = static_cast<MirOrientationMode>(window->attrib(mir_window_attrib_preferred_orientation));
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }

    return mode;
}

void mir_window_request_persistent_id(MirWindow* window, MirWindowIdCallback callback, void* context)
{
    mir_window_request_persistent_id_helper(window, callback, context);
}

namespace
{
void assign_surface_id_result(MirWindow*, MirWindowId* id, void* context)
{
    void** result_ptr = reinterpret_cast<void**>(context);
    *result_ptr = id;
}
}

MirWindowId* mir_window_request_persistent_id_sync(MirWindow* window)
{
    mir::require(mir_window_is_valid(window));

    MirWindowId* result = nullptr;
    if (auto wh = mir_window_request_persistent_id_helper(window, &assign_surface_id_result, &result))
        wh->wait_for_all();
    return result;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void mir_window_spec_add_render_surface(
    MirWindowSpec* spec,
    MirRenderSurface* render_surface,
    int logical_width, int logical_height,
    int displacement_x, int displacement_y)
try
{
    mir::require(spec && render_surface);
    auto rs = spec->connection->connection_surface_map()->render_surface(render_surface);

    if (rs->stream_id().as_value() < 0)
        BOOST_THROW_EXCEPTION(std::logic_error("Render surface holds no content."));
    ContentInfo info {
        {displacement_x, displacement_y},
        rs->stream_id().as_value(),
        mir::geometry::Size{ logical_width, logical_height }
    };

    if (spec->streams.is_set())
        spec->streams.value().push_back(info);
    else
        spec->streams = std::vector<ContentInfo>{info};
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

MirSurfaceSpec* mir_connection_create_spec_for_normal_surface(MirConnection* connection,
                                                              int width, int height,
                                                              MirPixelFormat format)
{
    auto spec = mir_create_normal_window_spec(connection, width, height);
    mir_window_spec_set_pixel_format(spec, format);
    return spec;
}

MirSurfaceSpec* mir_connection_create_spec_for_menu(MirConnection* connection,
                                                    int width, int height,
                                                    MirPixelFormat format,
                                                    MirSurface* parent,
                                                    MirRectangle* rect,
                                                    MirEdgeAttachment edge)
{
    auto spec = mir_create_menu_window_spec(connection, width, height, parent, rect, edge);
    mir_window_spec_set_pixel_format(spec, format);
    return spec;
}

MirSurfaceSpec* mir_connection_create_spec_for_tooltip(MirConnection* connection,
                                                       int width, int height,
                                                       MirPixelFormat format,
                                                       MirSurface* parent,
                                                       MirRectangle* rect)
{
    auto spec = mir_create_tip_window_spec(connection, width, height, parent, rect, mir_edge_attachment_any);
    mir_window_spec_set_pixel_format(spec, format);
    return spec;
}

MirSurfaceSpec* mir_connection_create_spec_for_tip(MirConnection* connection,
                                                   int width, int height,
                                                   MirPixelFormat format,
                                                   MirSurface* parent,
                                                   MirRectangle* rect,
                                                   MirEdgeAttachment edge)
{
    auto spec = mir_create_tip_window_spec(connection, width, height, parent, rect, edge);
    mir_window_spec_set_pixel_format(spec, format);
    return spec;
}

MirSurfaceSpec* mir_connection_create_spec_for_dialog(MirConnection* connection,
                                                      int width, int height,
                                                      MirPixelFormat format)
{
    auto spec = mir_create_dialog_window_spec(connection, width, height);
    mir_window_spec_set_pixel_format(spec, format);
    return spec;
}

MirSurfaceSpec* mir_connection_create_spec_for_input_method(MirConnection* connection,
                                                            int width, int height,
                                                            MirPixelFormat format)
{
    auto spec = mir_create_input_method_window_spec(connection, width, height);
    mir_window_spec_set_pixel_format(spec, format);
    return spec;
}

MirSurfaceSpec* mir_connection_create_spec_for_modal_dialog(MirConnection* connection,
                                                            int width, int height,
                                                            MirPixelFormat format,
                                                            MirSurface* parent)
{
    auto spec = mir_create_modal_dialog_window_spec(connection, width, height, parent);
    mir_window_spec_set_pixel_format(spec, format);
    return spec;
}

MirSurface* mir_surface_create_sync(MirSurfaceSpec* requested_specification)
{
    return mir_create_window_sync(requested_specification);
}

MirWaitHandle* mir_surface_create(MirSurfaceSpec* requested_specification,
                                  mir_surface_callback callback, void* context)
{
    return window_create_helper(requested_specification, callback, context);
}

void mir_surface_spec_set_name(MirSurfaceSpec* spec, char const* name)
{
    mir_window_spec_set_name(spec, name);
}

void mir_surface_spec_set_width(MirSurfaceSpec* spec, unsigned width)
{
    mir_window_spec_set_width(spec, width);
}

void mir_surface_spec_set_height(MirSurfaceSpec* spec, unsigned height)
{
    mir_window_spec_set_height(spec, height);
}

void mir_surface_spec_set_min_width(MirSurfaceSpec* spec, unsigned min_width)
{
    mir_window_spec_set_min_width(spec, min_width);
}

void mir_surface_spec_set_min_height(MirSurfaceSpec* spec, unsigned min_height)
{
    mir_window_spec_set_min_height(spec, min_height);
}

void mir_surface_spec_set_max_width(MirSurfaceSpec* spec, unsigned max_width)
{
    mir_window_spec_set_max_width(spec, max_width);
}

void mir_surface_spec_set_max_height(MirSurfaceSpec* spec, unsigned max_height)
{
    mir_window_spec_set_max_height(spec, max_height);
}

void mir_surface_spec_set_pixel_format(MirSurfaceSpec* spec, MirPixelFormat format)
{
    mir_window_spec_set_pixel_format(spec, format);
}

void mir_surface_spec_set_buffer_usage(MirSurfaceSpec* spec, MirBufferUsage usage)
{
    mir_window_spec_set_buffer_usage(spec, usage);
}

void mir_surface_spec_set_state(MirSurfaceSpec* spec, MirSurfaceState state)
{
    mir_window_spec_set_state(spec, static_cast<MirWindowState>(state));
}

void mir_surface_spec_set_fullscreen_on_output(MirSurfaceSpec* spec, uint32_t output_id)
{
    mir_window_spec_set_fullscreen_on_output(spec, output_id);
}

void mir_surface_spec_set_preferred_orientation(MirSurfaceSpec* spec, MirOrientationMode mode)
{
    mir_window_spec_set_preferred_orientation(spec, mode);
}

void mir_surface_spec_set_event_handler(MirSurfaceSpec* spec,
                                        mir_surface_event_callback callback,
                                        void* context)
{
    mir_window_spec_set_event_handler(spec, callback, context);
}

void mir_surface_spec_set_shell_chrome(MirSurfaceSpec* spec, MirShellChrome style)
{
    mir_window_spec_set_shell_chrome(spec, style);
}

void mir_surface_spec_release(MirSurfaceSpec* spec)
{
    mir_window_spec_release(spec);
}

MirSurfaceSpec* mir_create_surface_spec(MirConnection* connection)
{
    return mir_create_window_spec(connection);
}

MirSurfaceSpec* mir_connection_create_spec_for_changes(MirConnection* connection)
{
    return mir_create_window_spec(connection);
}

void mir_surface_apply_spec(MirSurface* surface, MirSurfaceSpec* spec)
{
    mir_window_apply_spec(surface, spec);
}

void mir_surface_spec_set_streams(MirSurfaceSpec* spec, MirBufferStreamInfo* streams, unsigned int size)
{
    mir_window_spec_set_streams(spec, streams, size);
}

void mir_surface_spec_set_input_shape(MirSurfaceSpec *spec, MirRectangle const* rectangles,
                                      size_t n_rects)
{
    mir_window_spec_set_input_shape(spec, rectangles, n_rects);
}

void mir_surface_spec_set_parent(MirSurfaceSpec* spec, MirSurface* parent)
{
    mir_window_spec_set_parent(spec, parent);
}

void mir_surface_spec_set_type(MirSurfaceSpec* spec, MirSurfaceType type)
{
    mir_window_spec_set_type(spec, static_cast<MirWindowType>(type));
}

void mir_surface_spec_set_width_increment(MirSurfaceSpec* spec, unsigned width_inc)
{
    mir_window_spec_set_width_increment(spec, width_inc);
}

void mir_surface_spec_set_height_increment(MirSurfaceSpec* spec, unsigned height_inc)
{
    mir_window_spec_set_height_increment(spec, height_inc);
}

void mir_surface_spec_set_min_aspect_ratio(MirSurfaceSpec* spec, unsigned width, unsigned height)
{
    mir_window_spec_set_min_aspect_ratio(spec, width, height);
}

void mir_surface_spec_set_max_aspect_ratio(MirSurfaceSpec* spec, unsigned width, unsigned height)
{
    mir_window_spec_set_max_aspect_ratio(spec, width, height);
}

void mir_surface_spec_set_pointer_confinement(MirSurfaceSpec* spec, MirPointerConfinementState state)
{
    mir_window_spec_set_pointer_confinement(spec, state);
}

void mir_surface_spec_set_placement(MirSurfaceSpec* spec,
                                    MirRectangle const* rect,
                                    MirPlacementGravity rect_gravity,
                                    MirPlacementGravity window_gravity,
                                    MirPlacementHints placement_hints,
                                    int offset_dx, int offset_dy)
{
    mir_window_spec_set_placement(spec,
                                  rect,
                                  rect_gravity,
                                  window_gravity,
                                  placement_hints,
                                  offset_dx,
                                  offset_dy);
}

bool mir_surface_spec_attach_to_foreign_parent(MirSurfaceSpec* spec,
                                               MirWindowId* parent,
                                               MirRectangle* attachment_rect,
                                               MirEdgeAttachment edge)
{
    return mir_window_spec_attach_to_foreign_parent(spec,
                                                    parent,
                                                    attachment_rect,
                                                    edge);
}

void mir_surface_set_event_handler(MirSurface* surface, mir_surface_event_callback callback, void* context)
{
    mir_window_set_event_handler(surface, callback, context);
}

bool mir_surface_is_valid(MirSurface* surface)
{
    return mir_window_is_valid(surface);
}

char const* mir_surface_get_error_message(MirSurface* surface)
{
    return mir_window_get_error_message(surface);
}

void mir_surface_get_parameters(MirSurface* surface, MirSurfaceParameters* parameters)
{
    mir_window_get_parameters(surface, parameters);
}

MirWaitHandle* mir_surface_release(
    MirSurface* surface,
    mir_surface_callback callback, void* context)
{
    return window_release_helper(surface, callback, context);
}

void mir_surface_release_sync(MirSurface* surface)
{
    mir_window_release_sync(surface);
}

MirSurfaceType mir_surface_get_type(MirSurface* surf)
{
    return static_cast<MirSurfaceType>(mir_window_get_type(surf));
}

MirWaitHandle* mir_surface_set_state(MirSurface* surf, MirSurfaceState state)
try
{
    mir::require(mir_window_is_valid(surf));
    return surf->configure(mir_window_attrib_state, state);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirSurfaceState mir_surface_get_state(MirSurface* surf)
{
    return static_cast<MirSurfaceState>(mir_window_get_state(surf));
}

MirOrientation mir_surface_get_orientation(MirSurface *surface)
{
    return mir_window_get_orientation(surface);
}

MirWaitHandle* mir_surface_set_swapinterval(MirSurface* surf, int interval)
{
    if (interval < 0)
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
    return mir_window_get_dpi(surf);
}

MirSurfaceFocusState mir_surface_get_focus(MirSurface* surf)
{
    return static_cast<MirSurfaceFocusState>(mir_window_get_focus_state(surf));
}

MirSurfaceVisibility mir_surface_get_visibility(MirSurface* surf)
{
    return static_cast<MirSurfaceVisibility>(mir_window_get_visibility(surf));
}

MirWaitHandle* mir_surface_configure_cursor(MirSurface* surface, MirCursorConfiguration const* cursor)
{
    return mir_configure_cursor_helper(surface, cursor);
}

MirOrientationMode mir_surface_get_preferred_orientation(MirSurface *surf)
{
    return mir_window_get_preferred_orientation(surf);
}

MirWaitHandle* mir_surface_set_preferred_orientation(MirSurface *surf, MirOrientationMode mode)
try
{
    mir::require(mir_window_is_valid(surf));
    return surf->set_preferred_orientation(mode);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

void mir_surface_raise(MirSurface* surf, MirCookie const* cookie)
{
    mir_window_raise(surf, cookie);
}

MirBufferStream* mir_surface_get_buffer_stream(MirSurface *surface)
{
    return mir_window_get_buffer_stream(surface);
}

MirWaitHandle* mir_surface_request_persistent_id(MirSurface* surface, MirWindowIdCallback callback, void* context)
{
    return mir_window_request_persistent_id_helper(surface, callback, context);
}

MirWindowId* mir_surface_request_persistent_id_sync(MirSurface *surface)
{
    MirWindowId* result = nullptr;
    if (auto wh = mir_window_request_persistent_id_helper(surface, &assign_surface_id_result, &result))
        wh->wait_for_all();
    return result;
}

#pragma GCC diagnostic pop

bool mir_persistent_id_is_valid(MirWindowId* id)
{
    return mir_window_id_is_valid(id);
}

void mir_persistent_id_release(MirWindowId* id)
{
    mir_window_id_release(id);
}

char const* mir_persistent_id_as_string(MirWindowId *id)
{
    return mir_window_id_as_string(id);
}

MirWindowId* mir_persistent_id_from_string(char const* id_string)
{
    return mir_window_id_from_string(id_string);
}

bool mir_window_id_is_valid(MirWindowId* id)
{
    return id != nullptr;
}

void mir_window_id_release(MirWindowId* id)
{
    delete id;
}

char const* mir_window_id_as_string(MirWindowId *id)
{
    return id->as_string().c_str();
}

MirWindowId* mir_window_id_from_string(char const* id_string)
{
    return new MirWindowId{id_string};
}

void mir_window_request_window_id(MirWindow* window, MirWindowIdCallback callback, void* context)
{
    mir_window_request_persistent_id_helper(window, callback, context);
}

MirWindowId* mir_window_request_window_id_sync(MirWindow* window)
{
    mir::require(mir_window_is_valid(window));

    MirWindowId* result = nullptr;
    if (auto wh = mir_window_request_persistent_id_helper(window, &assign_surface_id_result, &result))
        wh->wait_for_all();
    return result;
}
