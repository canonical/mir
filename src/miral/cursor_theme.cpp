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

#include <algorithm>

#define MIR_LOG_COMPONENT "miral"
#include <mir/log.h>

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

    server.add_configuration_option(option, "Colon separated cursor theme list (e.g. \"DMZ-Black\")", theme);

    server.override_the_cursor_images([&]
        {
            auto const themes = server.get_options()->get<std::string const>(option);

            for (auto i = std::begin(themes); i != std::end(themes); )
            {
                auto const j = std::find(i, std::end(themes), ':');

                std::string const theme{i, j};

                std::shared_ptr<mi::CursorImages> const xcursor_loader{std::make_shared<XCursorLoader>(theme)};

                if (has_default_cursor(*xcursor_loader))
                    return xcursor_loader;

                mir::log_warning("Failed to load cursor theme: %s", theme.c_str());

                if ((i = j) != std::end(themes)) ++i;
            }

            BOOST_THROW_EXCEPTION(std::runtime_error(("Failed to load cursor theme: " + themes).c_str()));
        });
}
