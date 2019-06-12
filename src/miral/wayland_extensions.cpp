/*
 * Copyright © 2018-2019 Canonical Ltd.
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

#include "miral/wayland_extensions.h"
#include "miral/window.h"

#include <mir/abnormal_exit.h>
#include <mir/frontend/wayland.h>
#include <mir/scene/session.h>
#include <mir/scene/surface.h>
#include <mir/server.h>
#include <mir/options/option.h>
#include <mir/options/configuration.h>

#include <set>

namespace mo = mir::options;

char const* const miral::zwlr_layer_shell_v1{"zwlr_layer_shell_v1"};
char const* const miral::zxdg_output_manager_v1{"zxdg_output_manager_v1"};

struct miral::WaylandExtensions::Self
{
    static auto vec2set(std::vector<std::string> vec) -> std::set<std::string>
    {
        return std::set<std::string>{vec.begin(), vec.end()};
    }

    Self()
        : default_extensions{vec2set(mir::frontend::get_standard_extensions())},
          supported_extensions{vec2set(mir::frontend::get_supported_extensions())}
    {
    }

    static auto parse_extensions_option(std::string extensions) -> std::set<std::string>
    {
        std::set<std::string> extension;
        extensions += ':';

        for (char const* start = extensions.c_str(); char const* end = strchr(start, ':'); start = end+1)
        {
            if (start != end)
                extension.insert(std::string{start, end});
        }

        return extension;
    }

    static auto serialize_colon_list(std::vector<std::string> extensions) -> std::string
    {
        bool at_start = true;
        std::string result;
        for (auto extension : extensions)
        {
            if (at_start)
                at_start = false;
            else
                result += ":";
            result += extension;
        }
        return result;
    }

    void validate(std::set<std::string> extensions) const
    {
        std::vector<std::string> errors;

        set_difference(
            begin(extensions), end(extensions),
            begin(supported_extensions), end(supported_extensions),
            std::inserter(errors, begin(errors)));

        if (!errors.empty())
        {
            throw mir::AbnormalExit{"Unsupported wayland extensions: " + serialize_colon_list(errors)};
        }
    }

    void add_extension(Builder const& builder)
    {
        wayland_extension_hooks.push_back(builder);
        supported_extensions.insert(builder.name);
    }

    void enable_extension(std::string name)
    {
        if (supported_extensions.find(name) == supported_extensions.end())
            mir::log_error("Attempted to enable unsupported extension " + name);
        else
            default_extensions.insert(name);
    }

    void disable_extension(std::string name)
    {
        if (supported_extensions.find(name) == supported_extensions.end())
            mir::log_error("Attempted to disable unsupported extension " + name);
        else
            default_extensions.erase(name);
    }

    std::vector<Builder> wayland_extension_hooks;
    /**
     * Extensions to enable by default if the user does not override with a command line options
     * This starts set to mir::frontend::get_standard_extensions()
     */
    std::set<std::string> default_extensions;
    /**
     * All the extensions this shell supports
     * This includes extensions returned by mir::frontend::get_supported_extensions() and any bespoke extensions added
     */
    std::set<std::string> supported_extensions;
    WaylandExtensions::Filter extensions_filter = [](Application const&, char const*) { return true; };
};


miral::WaylandExtensions::WaylandExtensions()
    : self{std::make_shared<Self>()}
{
}

miral::WaylandExtensions::WaylandExtensions(std::string const& default_value)
    : WaylandExtensions{}
{
    auto extensions = Self::parse_extensions_option(default_value);
    self->validate(extensions);
    self->default_extensions = extensions;
}

auto miral::WaylandExtensions::supported_extensions() const -> std::string
{
    std::vector<std::string> extensions{self->supported_extensions.begin(), self->supported_extensions.end()};
    return Self::serialize_colon_list(extensions);
}

auto miral::WaylandExtensions::recommended_extensions() -> std::string
{
    return Self::serialize_colon_list(mir::frontend::get_standard_extensions());
}

void miral::WaylandExtensions::operator()(mir::Server& server) const
{
    server.add_configuration_option(
        mo::wayland_extensions_opt,
        ("Wayland extensions to enable. [" + supported_extensions() + "]"),
        mir::OptionType::string);

    server.add_pre_init_callback([self=self, &server]
        {
            for (auto const& hook : self->wayland_extension_hooks)
            {
                struct FrigContext : Context
                {
                    wl_display* display_ = nullptr;
                    std::function<void(std::function<void()>&& work)> executor;

                    wl_display* display() const override
                    {
                        return display_;
                    }

                    void run_on_wayland_mainloop(std::function<void()>&& work) const override
                    {
                        executor(std::move(work));
                    }
                };

                auto frig = [build=hook.build, context=std::make_shared<FrigContext>()]
                    (wl_display* display, std::function<void(std::function<void()>&& work)> const& executor)
                {
                    context->display_ = display;
                    context->executor = executor;
                    return build(context.get());
                };

                server.add_wayland_extension(hook.name, std::move(frig));
            }

            server.set_wayland_extension_filter(self->extensions_filter);

            std::set<std::string> selected_extensions;
            if (server.get_options()->is_set(mo::wayland_extensions_opt))
            {
                selected_extensions = Self::parse_extensions_option(
                    server.get_options()->get<std::string>(mo::wayland_extensions_opt));
            }
            else
            {
                selected_extensions = self->default_extensions;
            }
            self->validate(selected_extensions);
            server.set_enabled_wayland_extensions(
                std::vector<std::string>{
                    selected_extensions.begin(),
                    selected_extensions.end()});
        });
}

miral::WaylandExtensions::~WaylandExtensions() = default;
miral::WaylandExtensions::WaylandExtensions(WaylandExtensions const&) = default;
auto miral::WaylandExtensions::operator=(WaylandExtensions const&) -> WaylandExtensions& = default;

void miral::WaylandExtensions::add_extension(Builder const& builder)
{
    self->add_extension(builder);
    self->enable_extension(builder.name);
}

void miral::WaylandExtensions::set_filter(miral::WaylandExtensions::Filter const& extension_filter)
{
    self->extensions_filter = extension_filter;
}

void miral::WaylandExtensions::add_extension_disabled_by_default(miral::WaylandExtensions::Builder const& builder)
{
    self->add_extension(builder);
}

auto miral::WaylandExtensions::enable(std::string name) -> WaylandExtensions&
{
    self->enable_extension(name);
    return *this;
}

auto miral::WaylandExtensions::disable(std::string name) -> WaylandExtensions&
{
    self->disable_extension(name);
    return *this;
}

auto miral::application_for(wl_client* client) -> Application
{
    return std::dynamic_pointer_cast<mir::scene::Session>(mir::frontend::get_session(client));
}

auto miral::application_for(wl_resource* resource) -> Application
{
    return std::dynamic_pointer_cast<mir::scene::Session>(mir::frontend::get_session(resource));
}

auto miral::window_for(wl_resource* surface) -> Window
{
    if (auto const& app = application_for(surface))
    {
        return {app, std::dynamic_pointer_cast<mir::scene::Surface>(mir::frontend::get_window(surface))};
    }
    else
    {
        return {};
    }
}
