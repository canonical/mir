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

#include <mir/server.h>
#include <mir/options/option.h>
#include <mir/options/configuration.h>

#include <set>
#include <mir/abnormal_exit.h>
#include <miral/wayland_extensions.h>

namespace mo = mir::options;

struct miral::WaylandExtensions::Self
{
    Self(std::string const& default_value) : default_value{default_value}
    {
        available_extensions += ":zwlr_layer_shell_v1:";
        validate(default_value);
    }

    void callback(mir::Server& server) const
    {
        validate(server.get_options()->get<std::string>(mo::wayland_extensions_opt));
        // TODO pass bespoke stuff into server!
    }

    std::string default_value;
    std::string available_extensions = mo::wayland_extensions_value;

    void validate(std::string extensions) const
    {
        std::set<std::string> selected_extension;
        auto extensions_ = extensions + ':';

        for (char const* start = extensions_.c_str(); char const* end = strchr(start, ':'); start = end+1)
        {
            if (start != end)
                selected_extension.insert(std::string{start, end});
        }

        std::set<std::string> supported_extension;

        for (char const* start = available_extensions.c_str(); char const* end = strchr(start, ':'); start = end+1)
        {
            if (start != end)
                supported_extension.insert(std::string{start, end});
        }

        std::set<std::string> errors;

        set_difference(begin(selected_extension), end(selected_extension),
                       begin(supported_extension), end(supported_extension),
                       std::inserter(errors, begin(errors)));

        if (!errors.empty())
        {
            throw mir::AbnormalExit{"Unsupported wayland extensions in: " + extensions};
        }
    }

    std::vector<Builder> wayland_extension_hooks;

    void add_extension(Builder const& builder)
    {
        wayland_extension_hooks.push_back(builder);
        available_extensions += builder.name + ":";
    }

    void add_to_default(Builder const& builder)
    {
        default_value += ":" + builder.name;
    }

    WaylandExtensions::Filter extensions_filter = [](Application const&, char const*) { return true; };
};

miral::WaylandExtensions::WaylandExtensions() :
    WaylandExtensions{mo::wayland_extensions_value}
{
}

miral::WaylandExtensions::WaylandExtensions(std::string const& default_value) :
    self{std::make_shared<Self>(default_value)}
{
}

auto miral::WaylandExtensions::supported_extensions() const -> std::string
{
    return self->available_extensions.substr(0, self->available_extensions.size()-1);
}

auto miral::WaylandExtensions::recommended_extensions() -> std::string
{
    return mo::wayland_extensions_value;
}

void miral::WaylandExtensions::operator()(mir::Server& server) const
{
    self->validate(self->default_value);
    server.add_configuration_option(
        mo::wayland_extensions_opt,
        ("Wayland extensions to enable. [" + supported_extensions() + "]"),
        self->default_value);

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
        });

    server.add_init_callback([this, &server]{ self->callback(server); });
}

miral::WaylandExtensions::~WaylandExtensions() = default;
miral::WaylandExtensions::WaylandExtensions(WaylandExtensions const&) = default;
auto miral::WaylandExtensions::operator=(WaylandExtensions const&) -> WaylandExtensions& = default;

void miral::WaylandExtensions::add_extension(Builder const& builder)
{
    self->add_extension(builder);
    self->add_to_default(builder);
}

void miral::WaylandExtensions::set_filter(miral::WaylandExtensions::Filter const& extension_filter)
{
    self->extensions_filter = extension_filter;
}

void miral::WaylandExtensions::add_extension_disabled_by_default(miral::WaylandExtensions::Builder const& builder)
{
    self->add_extension(builder);
}
