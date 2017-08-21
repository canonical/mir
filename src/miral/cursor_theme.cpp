/*
 * Copyright © 2015-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
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

#include "miral/cursor_theme.h"
#include "xcursor_loader.h"

#include <mir/options/option.h>
#include <mir/server.h>
#include <mir_toolkit/cursors.h>

#include <boost/throw_exception.hpp>

namespace mi = mir::input;

namespace
{
bool has_default_cursor(mi::CursorImages& images)
{
    return !!images.image(mir_default_cursor_name, mi::default_cursor_size);
}
}

miral::CursorTheme::CursorTheme(std::string const& theme) :
    theme{theme}
{
}

miral::CursorTheme::~CursorTheme() = default;

void miral::CursorTheme::operator()(mir::Server& server) const
{
    static char const* const option = "cursor-theme";

    server.add_configuration_option(option, "Cursor theme (e.g. \"DMZ-Black\")", theme);

    server.override_the_cursor_images([&]
        {
            auto const theme = server.get_options()->get<std::string const>(option);

            std::shared_ptr<mi::CursorImages> const xcursor_loader{std::make_shared<XCursorLoader>(theme)};

            if (has_default_cursor(*xcursor_loader))
                return xcursor_loader;

            BOOST_THROW_EXCEPTION(std::runtime_error(("Failed to load cursor theme: " + theme).c_str()));
        });
}
