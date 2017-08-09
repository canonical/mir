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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#define MIR_LOG_COMPONENT "MirCursorAPI"

#include "mir_toolkit/mir_cursor_configuration.h"
#include "cursor_configuration.h"

#include "mir/uncaught.h"

#include <memory>

// Cursor names are from CSS3: https://www.w3.org/TR/css-ui-3/#propdef-cursor
extern "C" char const* const mir_default_cursor_name = "default";
extern "C" char const* const mir_disabled_cursor_name = "none";
extern "C" char const* const mir_arrow_cursor_name = "default";
extern "C" char const* const mir_busy_cursor_name = "wait";
extern "C" char const* const mir_caret_cursor_name = "text";
extern "C" char const* const mir_pointing_hand_cursor_name = "pointer";
extern "C" char const* const mir_open_hand_cursor_name = "grab";
extern "C" char const* const mir_closed_hand_cursor_name = "grabbing";
extern "C" char const* const mir_horizontal_resize_cursor_name = "ew-resize";
extern "C" char const* const mir_vertical_resize_cursor_name = "ns-resize";
extern "C" char const* const mir_diagonal_resize_bottom_to_top_cursor_name = "ne-resize";
extern "C" char const* const mir_diagonal_resize_top_to_bottom_cursor_name = "se-resize";
extern "C" char const* const mir_omnidirectional_resize_cursor_name = "move";
extern "C" char const* const mir_vsplit_resize_cursor_name = "row-resize";
extern "C" char const* const mir_hsplit_resize_cursor_name = "col-resize";
extern "C" char const* const mir_crosshair_cursor_name = "crosshair";

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

MirCursorConfiguration::MirCursorConfiguration(char const* name) :
    name{name ? name : std::string()},
    stream(nullptr),
    surface(nullptr)
{
}

MirCursorConfiguration::MirCursorConfiguration(MirBufferStream const* stream, int hotspot_x, int hotspot_y) :
    stream(stream),
    hotspot_x(hotspot_x),
    hotspot_y(hotspot_y),
    surface(nullptr)
{
}


MirCursorConfiguration::MirCursorConfiguration(MirRenderSurface const* surface, int hotspot_x, int hotspot_y) :
    stream(nullptr),
    hotspot_x(hotspot_x),
    hotspot_y(hotspot_y),
    surface(surface)
{
}

void mir_cursor_configuration_destroy(MirCursorConfiguration *cursor)
{
    delete cursor;
}

MirCursorConfiguration* mir_cursor_configuration_from_name(char const* name)
{
    try 
    {
        return new MirCursorConfiguration(name);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }
}

MirCursorConfiguration* mir_cursor_configuration_from_buffer_stream(MirBufferStream const* stream, int hotspot_x,
    int hotspot_y)
{
    try 
    {
        return new MirCursorConfiguration(stream, hotspot_x, hotspot_y);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }
}
#pragma GCC diagnostic pop
