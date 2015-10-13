/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_cursor_images.h"
#include "xcursor_loader.h"

#include "mir/server.h"
#include "mir/options/option.h"

#include <mir_toolkit/cursors.h>

namespace mi = mir::input;

namespace
{
char const* const xcursor_theme = "x-cursor-theme";
char const* const xcursor_description = "X Cursor theme to load [default, DMZ-White, DMZ-Black, ...]";

bool has_default_cursor(mi::CursorImages& images)
{
    if (images.image(mir_default_cursor_name, mi::default_cursor_size))
        return true;
    return false;
}
}

void mir::examples::add_x_cursor_images(Server& server)
{
    server.add_configuration_option(xcursor_theme, xcursor_description, "default");

    server.override_the_cursor_images([&]
        {
            auto const theme = server.get_options()->get<std::string>(xcursor_theme);

            std::shared_ptr<mi::CursorImages> const xcursor_loader{std::make_shared<XCursorLoader>(theme)};

            if (has_default_cursor(*xcursor_loader))
                return xcursor_loader;
            else
                return std::shared_ptr<mi::CursorImages>{};
        });
}
