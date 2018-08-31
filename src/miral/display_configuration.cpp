/*
 * Copyright © 2018 Canonical Ltd.
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

#include "miral/display_configuration.h"
#include "miral/runner.h"
#include "miral/command_line_option.h"
#include "static_display_config.h"

#include <mir/server.h>
#include <mir/log.h>

#include <unistd.h>

#include <fstream>
#include <sstream>

struct miral::DisplayConfiguration::Self : StaticDisplayConfig
{
    Self(std::string const& name) : name{name} {}
    std::string const name;

    void dump_config(std::function<void(std::ostream&)> const& print_template_config) override
    {
        std::string config_dir;

        if (auto config_home = getenv("XDG_CONFIG_HOME"))
            config_dir = config_home;
        else if (auto home = getenv("HOME"))
            (config_dir = home) += "/.config";

        if (config_dir.empty())
        {
            mir::log_debug("Neither XDG_CONFIG_HOME or HOME is set");
        }
        else
        {
            auto const filename = config_dir + "/" + name;

            if (access(filename.c_str(), F_OK))
            {
                std::ofstream out{filename};
                print_template_config(out);

                if (!out)
                    mir::log_debug("Failed writing display configuration template: %s", filename.c_str());
            }
        }

        StaticDisplayConfig::dump_config(print_template_config);
    }
};

miral::DisplayConfiguration::DisplayConfiguration(MirRunner const& mir_runner) :
    self{std::make_shared<Self>(mir_runner.display_config_file())}
{
    std::string config_roots;

    if (auto config_home = getenv("XDG_CONFIG_HOME"))
        (config_roots = config_home) += ":";
    else if (auto home = getenv("HOME"))
        (config_roots = home) += "/.config:";

    if (auto config_dirs = getenv("XDG_CONFIG_DIRS"))
        config_roots += config_dirs;
    else
        config_roots += "/etc/xdg";

    std::istringstream config_stream(config_roots);

    /* Read options from config files */
    for (std::string config_root; getline(config_stream, config_root, ':');)
    {
        auto const& filename = config_root + "/" + self->name;

        if (std::ifstream config_file{filename})
        {
            self->load_config(config_file, "ERROR: in display configuration file: '" + filename + "' : ");
            break;
        }
    }
}

void miral::DisplayConfiguration::select_layout(std::string const& layout)
{
    self->select_layout(layout);
}

void miral::DisplayConfiguration::operator()(mir::Server& server) const
{
    namespace mg = mir::graphics;

    server.wrap_display_configuration_policy([this](std::shared_ptr<mg::DisplayConfigurationPolicy> const&)
        -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            return self;
        });
}

auto miral::DisplayConfiguration::layout_option() -> miral::CommandLineOption
{
    return pre_init(CommandLineOption{
        [this](std::string const& layout) { select_layout(layout); },
        "display-layout",
        "Display configuration layout from `" + self->name + "'\n"
        "(Found in $XDG_CONFIG_HOME or $HOME/.config, followed by $XDG_CONFIG_DIRS)",
        "default"});
}

auto miral::DisplayConfiguration::list_layouts() -> std::vector<std::string>
{
    return self->list_layouts();
}

miral::DisplayConfiguration::~DisplayConfiguration() = default;

miral::DisplayConfiguration::DisplayConfiguration(miral::DisplayConfiguration const&) = default;

auto miral::DisplayConfiguration::operator=(miral::DisplayConfiguration const&) -> miral::DisplayConfiguration& = default;
