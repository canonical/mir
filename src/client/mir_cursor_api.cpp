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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#define MIR_LOG_COMPONENT "MirCursorAPI"

#include "mir_toolkit/mir_cursor_configuration.h"
#include "cursor_configuration.h"

#include "uncaught.h"

#include <memory>

char const *const mir_default_cursor_name = "default";
char const *const mir_disabled_cursor_name = "disabled";
char const* const mir_arrow_cursor_name = "arrow";
char const* const mir_busy_cursor_name = "busy";
char const* const mir_caret_cursor_name = "caret";
char const* const mir_pointing_hand_cursor_name = "pointing-hand";
char const* const mir_open_hand_cursor_name = "open-hand";
char const* const mir_closed_hand_cursor_name = "closed-hand";
char const* const mir_horizontal_resize_cursor_name = "horizontal-resize";
char const* const mir_vertical_resize_cursor_name = "vertical-resize";
char const* const mir_diagonal_resize_bottom_to_top_cursor_name = "diagonal-resize-bottom-to-top";
char const* const mir_diagonal_resize_top_to_bottom_cursor_name = "diagonal-resize-top_to_bottom";
char const* const mir_omnidirectional_resize_cursor_name = "omnidirectional-resize";
char const* const mir_vsplit_resize_cursor_name = "vsplit-resize";
char const* const mir_hsplit_resize_cursor_name = "hsplit-resize";

MirCursorConfiguration::MirCursorConfiguration(char const* name) :
    name{name ? name : std::string()}
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
