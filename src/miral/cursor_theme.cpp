/*
 * Copyright © Canonical Ltd.
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
 */

#include <miral/cursor_theme.h>
#include <miral/live_config.h>
#include "xcursor_loader.h"

#include <mir/input/cursor_images.h>
#include <mir/input/runtime_cursor_images.h>
#include <mir/options/option.h>
#include <mir/server.h>
#include <mir_toolkit/cursors.h>

#include <boost/throw_exception.hpp>

#include <algorithm>
#include <functional>
#include <memory>

#define MIR_LOG_COMPONENT "miral"
#include <mir/log.h>

namespace mi = mir::input;

namespace
{
bool has_default_cursor(mi::CursorImages& images)
{
    return !!images.image(mir_default_cursor_name, mi::default_cursor_size);
}

auto try_load_theme(std::string const& themes) -> std::shared_ptr<mi::CursorImages>
{
    for (auto i = std::begin(themes); i != std::end(themes); )
    {
        auto const j = std::find(i, std::end(themes), ':');

        std::string const theme{i, j};

        std::shared_ptr<mi::CursorImages> const xcursor_loader{
            std::make_shared<miral::XCursorLoader>(theme)};

        if (has_default_cursor(*xcursor_loader))
            return xcursor_loader;

        mir::log_warning("Failed to load cursor theme: %s", theme.c_str());

        if ((i = j) != std::end(themes)) ++i;
    }

    return {};
}
}

struct miral::CursorTheme::Self
{
    explicit Self(std::string const& default_theme) : default_theme{default_theme} {}

    std::string const default_theme;

    // Set during operator()(Server&) — allows set_theme() to swap the inner CursorImages
    std::weak_ptr<mi::RuntimeCursorImages> runtime_cursor_images;
};

miral::CursorTheme::CursorTheme(std::string const& theme) :
    self{std::make_shared<Self>(theme)}
{
}

miral::CursorTheme::CursorTheme(std::string const& theme, live_config::Store& config_store) :
    self{std::make_shared<Self>(theme)}
{
    config_store.add_string_attribute(
        {"cursor", "theme"},
        "Colon separated cursor theme list",
        [this](live_config::Key const& key, std::optional<std::string_view> val)
        {
            if (val && !val->empty())
            {
                set_theme(std::string{*val});
            }
            else
            {
                set_theme(self->default_theme);
            }
        });
}

miral::CursorTheme::~CursorTheme() = default;

void miral::CursorTheme::operator()(mir::Server& server) const
{
    static char const* const option = "cursor-theme";

    server.add_configuration_option(option, "Colon separated cursor theme list, e.g. default:DMZ-Black.", self->default_theme);

    server.override_the_cursor_images([self = this->self, &server]
        {
            auto const themes = server.get_options()->get<std::string const>(option);

            auto loaded = try_load_theme(themes);

            if (!loaded)
            {
                mir::log_warning("Failed to load any cursor theme! Using built-in cursors.");
                return std::shared_ptr<mi::CursorImages>{};
            }

            // Wrap the loaded theme in a RuntimeCursorImages proxy so that
            // set_theme() can swap the inner CursorImages at runtime.
            auto runtime = std::make_shared<mi::RuntimeCursorImages>(std::move(loaded));
            self->runtime_cursor_images = runtime;
            return std::static_pointer_cast<mi::CursorImages>(runtime);
        });
}

void miral::CursorTheme::set_theme(std::string const& theme) const
{
    auto runtime = self->runtime_cursor_images.lock();
    if (!runtime)
    {
        mir::log_warning("CursorTheme::set_theme() called before server initialization");
        return;
    }

    auto loaded = try_load_theme(theme);
    if (!loaded)
    {
        mir::log_warning("Failed to load cursor theme: %s", theme.c_str());
        return;
    }

    runtime->set_cursor_images(std::move(loaded));
}
