/*
 * Copyright © Canonical Ltd.
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

#include <algorithm>
#include <mutex>
#include <set>
#include <map>
#include <vector>
#include <iterator>

namespace mo = mir::options;

char const* const miral::WaylandExtensions::zwlr_layer_shell_v1{"zwlr_layer_shell_v1"};
char const* const miral::WaylandExtensions::zxdg_output_manager_v1{"zxdg_output_manager_v1"};
char const* const miral::WaylandExtensions::zwlr_foreign_toplevel_manager_v1{"zwlr_foreign_toplevel_manager_v1"};
char const* const miral::WaylandExtensions::zwp_virtual_keyboard_manager_v1{"zwp_virtual_keyboard_manager_v1"};
char const* const miral::WaylandExtensions::zwp_input_method_v1{"zwp_input_method_v1"};
char const* const miral::WaylandExtensions::zwp_input_panel_v1{"zwp_input_panel_v1"};
char const* const miral::WaylandExtensions::zwp_input_method_manager_v2{"zwp_input_method_manager_v2"};
char const* const miral::WaylandExtensions::zwlr_screencopy_manager_v1{"zwlr_screencopy_manager_v1"};
char const* const miral::WaylandExtensions::zwlr_virtual_pointer_manager_v1{"zwlr_virtual_pointer_manager_v1"};
char const* const miral::WaylandExtensions::ext_session_lock_manager_v1{"ext_session_lock_manager_v1"};
char const* const miral::WaylandExtensions::ext_data_control_manager_v1{"ext_data_control_manager_v1"};

namespace
{
auto vec2set(std::vector<std::string> vec) -> std::set<std::string>
{
    return std::set<std::string>{vec.begin(), vec.end()};
}

// If a user inadvertently supplies more than one miral::WaylandExtensions to
// MirRunner::run_with() then the implementation here would quietly break.
// To avoid this we track the extensions against all servers in the process.
// See: https://github.com/canonical/mir/issues/875
struct StaticExtensionTracker
{
    static void add_server_extension(mir::Server* server, void* extension)
    {
        std::lock_guard lock{mutex};

        for (auto const& x : extensions)
        {
            if (x.server == server)
            {
                if (x.extension == extension)
                {
                    return;
                }
                else
                {
                    throw mir::AbnormalExit{"ERROR: multiple miral::WaylandExtensions registered with server"};
                }
            }
        }
        extensions.push_back({server, extension});
    }

    static void remove_extension(void* extension)
    {
        std::lock_guard lock{mutex};

        extensions.erase(
            remove_if(begin(extensions), end(extensions), [extension](auto& x) { return x.extension == extension; }),
            end(extensions));
    }

private:
    // It's extremely unlikely that there's more than a couple, so lock contention
    // is a non-issue and a vector is a suitable data.
    struct ExtensionTracker{ mir::Server* server; void* extension; };
    static std::mutex mutex;
    static std::vector<ExtensionTracker> extensions;
};

decltype(StaticExtensionTracker::mutex)       StaticExtensionTracker::mutex;
decltype(StaticExtensionTracker::extensions)  StaticExtensionTracker::extensions;

auto map_to_global_name(std::string const& extension) -> std::string
{
    if (extension == "zwp_input_method_v2")
    {
        return "zwp_input_method_manager_v2";
    }
    else if (extension == "zwp_virtual_keyboard_v1")
    {
        return "zwp_virtual_keyboard_manager_v1";
    }
    else
    {
        return extension;
    }
}

/// The extension names given to Mir may not always be the name of the relevant global, but Mir needs the global name
auto map_to_global_names(std::set<std::string> const& extensions) -> std::set<std::string>
{
    std::set<std::string> result;
    for (auto const& extension : extensions)
        result.insert(map_to_global_name(extension));
    return result;
}
}

struct miral::WaylandExtensions::EnableInfo::Self
{
    Application const app;
    const char* const name;
    std::optional<bool> const user_preference;
};

miral::WaylandExtensions::EnableInfo::EnableInfo(
    Application const& app,
    const char* name,
    std::optional<bool> user_preference)
    : self{std::unique_ptr<Self>(new Self{app, name, user_preference})}
{
}

auto miral::WaylandExtensions::EnableInfo::app() const -> Application const&
{
    return self->app;
}

auto miral::WaylandExtensions::EnableInfo::name() const -> const char*
{
    return self->name;
}

auto miral::WaylandExtensions::EnableInfo::user_preference() const -> std::optional<bool>
{
    return self->user_preference;
}

struct miral::WaylandExtensions::Self
{

    Self()
        : recommended_extensions{WaylandExtensions::recommended()},
          default_extensions{recommended_extensions},
          supported_extensions{WaylandExtensions::supported()}
    {
    }

    ~Self()
    {
        StaticExtensionTracker::remove_extension(this);
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

    static auto serialize_list(std::vector<std::string> extensions, std::string const& joiner) -> std::string
    {
        bool at_start = true;
        std::string result;
        for (auto extension : extensions)
        {
            if (at_start)
                at_start = false;
            else
                result += joiner;
            result += extension;
        }
        return result;
    }

    void validate(std::set<std::string> const& extensions) const
    {
        std::vector<std::string> errors;

        set_difference(
            begin(extensions), end(extensions),
            begin(supported_extensions), end(supported_extensions),
            std::inserter(errors, begin(errors)));

        if (!errors.empty())
        {
            throw mir::AbnormalExit{
                "Unsupported wayland extensions: " +
                serialize_list(errors, ":") +
                ". " +
                describe_extensions()};
        }
    }

    void add_extension(Builder const& builder)
    {
        if (supported_extensions.find(builder.name) != supported_extensions.end())
        {
            throw mir::AbnormalExit{"Attempt to duplicate wayland extensions: " + builder.name};
        }
        wayland_extension_hooks.push_back(builder);
        supported_extensions.insert(builder.name);
    }

    void enable_extension(std::string name)
    {
        if (supported_extensions.find(name) == supported_extensions.end())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("Attempted to enable unsupported extension " + name));
        }
        else
        {
            default_extensions.insert(name);
            conditional_extensions.erase(name);
        }
    }

    void disable_extension(std::string name)
    {
        if (supported_extensions.find(name) == supported_extensions.end())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("Attempted to disable unsupported extension " + name));
        }
        else
        {
            default_extensions.erase(name);
            conditional_extensions.erase(name);
        }
    }

    void conditionally_enable(std::string name, EnableCallback const& callback)
    {
        if (supported_extensions.find(name) == supported_extensions.end())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error(
                "Attempted to conditionally enable unsupported extension " + name));
        }
        else
        {
            default_extensions.erase(name);
            conditional_extensions[name] = callback;
        }
    }

    void init_server(mir::Server& server)
    {
        // Add the shell-custom extensions.
        for (auto const& hook : wayland_extension_hooks)
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

        // Next, parse the command line options to see which are enabled for all
        // wayland clients by default.
        //
        // If a supported extension does not appear in the list, add it to the
        // list of manually disabled extensions.
        std::set<std::string> enabled_extensions;
        std::set<std::string> manually_enabled_extensions; // Note that this set only exists to validate `user_pref` later on.
        std::set<std::string> manually_disabled_extensions;
        if (server.get_options()->is_set(mo::wayland_extensions_opt))
        {
            manually_enabled_extensions = Self::parse_extensions_option(
                server.get_options()->get<std::string>(mo::wayland_extensions_opt));
            enabled_extensions = manually_enabled_extensions;
            for (auto const& ext : supported_extensions)
            {
                if (!manually_enabled_extensions.contains(ext))
                    manually_disabled_extensions.insert(ext);
            }
        }
        else
        {
            enabled_extensions = default_extensions;
        }

        // Next, add the additional extensions that are enabled by default.
        if (server.get_options()->is_set(mo::add_wayland_extensions_opt))
        {
            auto const value = server.get_options()->get<std::string>(mo::add_wayland_extensions_opt);
            auto const added = value == "all" ? supported_extensions : Self::parse_extensions_option(value);
            for (auto const& extension : added)
            {
                enabled_extensions.insert(extension);
                manually_enabled_extensions.insert(extension);
                manually_disabled_extensions.erase(extension);
            }
        }

        // Next, manage the explicitly dropped extensions.
        if (server.get_options()->is_set(mo::drop_wayland_extensions_opt))
        {
            auto const dropped = Self::parse_extensions_option(
                server.get_options()->get<std::string>(mo::drop_wayland_extensions_opt));
            for (auto const& extension : dropped)
            {
                enabled_extensions.erase(extension);
                manually_enabled_extensions.erase(extension);
                manually_disabled_extensions.insert(extension);
            }
        }

        // Now that we know which extensions should be available, we are ready to
        // set the policy for each extension.
        enabled_extensions = map_to_global_names(enabled_extensions);
        manually_disabled_extensions = map_to_global_names(manually_disabled_extensions);
        validate(enabled_extensions);

        // Set up the always-enabled extensions.
        for (auto const& extension : enabled_extensions)
        {
            server.set_wayland_extension_policy(extension, [](auto const&, const char*)
            {
                return true;
            });
        }

        // Set up the missing extensions that do not appear in the always-enabled list.
        for (auto const& extension : supported_extensions)
        {
            if (!enabled_extensions.contains(extension))
            {
                server.set_wayland_extension_policy(extension, [](auto const&, const char*)
                {
                    return false;
                });
            }
        }

        // Set up the manually disabled extensions.
        for (auto const& extension : manually_disabled_extensions)
        {
            server.set_wayland_extension_policy(extension, [](auto const&, const char*)
            {
                return false;
            });
        }

        // Set up the conditional extensions.
        for (auto const& [extension, filter] : conditional_extensions)
        {
            server.set_wayland_extension_policy(map_to_global_name(extension), [
                manually_enabled_extensions=manually_enabled_extensions,
                manually_disabled_extensions=manually_disabled_extensions,
                filter](auto const& app, const char* protocol)
            {
                std::optional<bool> user_pref;
                if (manually_enabled_extensions.contains(protocol))
                {
                    user_pref = true;
                }
                else if (manually_disabled_extensions.contains(protocol))
                {
                    user_pref = false;
                }

                return filter(EnableInfo{ app, protocol, user_pref});
            });
        }
    }

    auto describe_extensions() const -> std::string
    {
        std::vector<std::string> non_default_extensions;
        for (auto const& extension : supported_extensions)
        {
            if (default_extensions.find(extension) == default_extensions.end())
            {
                non_default_extensions.push_back(extension);
            }
        }
        std::string const joiner = "\n - ";
        return "Default extensions:" + joiner +
            serialize_list({default_extensions.begin(), default_extensions.end()}, joiner) +
            "\nAdditional supported extensions:" + joiner +
            serialize_list(non_default_extensions, joiner);
    }

    std::set<std::string> const recommended_extensions;
    std::vector<Builder> wayland_extension_hooks;
    std::map<std::string, EnableCallback> conditional_extensions;
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
};


miral::WaylandExtensions::WaylandExtensions()
    : self{std::make_shared<Self>()}
{
}

void miral::WaylandExtensions::operator()(mir::Server& server) const
{
    StaticExtensionTracker::add_server_extension(&server, self.get());

    server.add_configuration_option(
        mo::wayland_extensions_opt,
        ("Colon separated list of Wayland extensions to enable. "
         "If used, default extensions will NOT be enabled unless specified. " +
         self->describe_extensions()),
        mir::OptionType::string);

    server.add_configuration_option(
        mo::add_wayland_extensions_opt,
        ("Colon separated list of Wayland extensions to enable in addition to default extensions. "
         "Use `all` to enable all supported extensions."),
        mir::OptionType::string);

    server.add_configuration_option(
        mo::drop_wayland_extensions_opt,
        ("Colon separated list of Wayland extensions to disable."),
        mir::OptionType::string);

    server.add_pre_init_callback([self=self, &server]
        {
            self->init_server(server);
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

void miral::WaylandExtensions::add_extension_disabled_by_default(miral::WaylandExtensions::Builder const& builder)
{
    self->add_extension(builder);
}

auto miral::WaylandExtensions::recommended() -> std::set<std::string>
{
    return vec2set(mir::frontend::get_standard_extensions());
}

auto miral::WaylandExtensions::supported() -> std::set<std::string>
{
    return vec2set(mir::frontend::get_supported_extensions());
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

auto miral::WaylandExtensions::conditionally_enable(
    std::string name,
    EnableCallback const& callback) -> WaylandExtensions&
{
    self->conditionally_enable(name, callback);
    return *this;
}

auto miral::WaylandExtensions::all_supported() const -> std::set<std::string>
{
    return self->supported_extensions;
}

auto miral::application_for(wl_client* client) -> Application
{
    return mir::frontend::get_session(client);
}

auto miral::application_for(wl_resource* resource) -> Application
{
    return mir::frontend::get_session(resource);
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
