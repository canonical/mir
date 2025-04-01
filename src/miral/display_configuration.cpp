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

#include "miral/display_configuration.h"
#include "miral/runner.h"
#include "miral/command_line_option.h"
#include "static_display_config.h"

#include <mir/server.h>

#include <boost/throw_exception.hpp>

#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

using namespace std::string_literals;

namespace
{
class YamlDisplayConfigurationNode : public miral::DisplayConfigurationNode
{
public:
    explicit YamlDisplayConfigurationNode(YAML::Node const& node)
        : node(node)
    {
    }

    void for_each(std::function<void(std::unique_ptr<DisplayConfigurationNode>)> const& f) const override
    {
        if (!node.IsSequence())
            return;

        for (auto const& item : node)
            f(std::make_unique<YamlDisplayConfigurationNode>(item));
    }

    std::optional<std::string> as_string() const override
    {
        if (node.IsScalar())
            return node.Scalar();


        return std::nullopt;
    }

    std::optional<int> as_int() const override
    {
        try
        {
            return node.as<int>();
        }
        catch (YAML::Exception const&)
        {
            return std::nullopt;
        }
    }

    std::optional<std::unique_ptr<DisplayConfigurationNode>> at(std::string const& key) const override
    {
        if (!node[key])
            return std::nullopt;

        return std::make_unique<YamlDisplayConfigurationNode>(node[key]);
    }

private:
    YAML::Node node;
};
}

class miral::DisplayConfiguration::Self : public ReloadingYamlFileDisplayConfig
{
public:

    using ReloadingYamlFileDisplayConfig::ReloadingYamlFileDisplayConfig;

    void load()
    {
        std::string config_roots;

        if (auto config_home = getenv("XDG_CONFIG_HOME"))
        {
            (config_roots = config_home) += ":";
            config_path(config_home);
        }
        else if (auto home = getenv("HOME"))
        {
            (config_roots = home) += "/.config:";
            config_path(home + "/.config"s);
        }

        if (auto config_dirs = getenv("XDG_CONFIG_DIRS"))
            config_roots += config_dirs;
        else
            config_roots += "/etc/xdg";

        std::istringstream config_stream(config_roots);

        /* Read options from config files */
        for (std::string config_root; getline(config_stream, config_root, ':');)
        {
            auto const& filename = config_root + "/" + basename;

            if (std::ifstream config_file{filename})
            {
                load_config(config_file, filename);
                config_path(config_root);
                break;
            }
        }
    }

};

miral::DisplayConfiguration::DisplayConfiguration(MirRunner const& mir_runner) :
    self{std::make_shared<Self>(mir_runner.display_config_file())}
{
}

void miral::DisplayConfiguration::select_layout(std::string const& layout)
{
    self->select_layout(layout);
}

void miral::DisplayConfiguration::operator()(mir::Server& server) const
{
    self->load();

    namespace mg = mir::graphics;

    server.wrap_display_configuration_policy([this](std::shared_ptr<mg::DisplayConfigurationPolicy> const&)
        -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            return self;
        });

    server.add_init_callback([self=self, &server]
        {
            self->init_auto_reload(server);
        });
}

auto miral::DisplayConfiguration::layout_option() -> miral::ConfigurationOption
{
    return pre_init(ConfigurationOption{
        [this](std::string const& layout) { select_layout(layout); self->check_for_layout_override(); },
        "display-layout",
        "Display configuration layout from `" + self->basename + "'\n"
        "(Found in $XDG_CONFIG_HOME or $HOME/.config, followed by $XDG_CONFIG_DIRS)",
        "default"});
}

auto miral::DisplayConfiguration::list_layouts() -> std::vector<std::string>
{
    return self->list_layouts();
}

auto miral::DisplayConfiguration::layout_userdata() const -> std::shared_ptr<void>
{
    return self->layout_userdata();
}

void miral::DisplayConfiguration::add_output_attribute(std::string const& key)
{
    self->add_output_attribute(key);
}

void miral::DisplayConfiguration::set_layout_userdata_builder(std::function<std::shared_ptr<void>(
    std::string const& layout,
    std::unique_ptr<DisplayConfigurationNode> value)> const& builder) const
{
    self->set_layout_userdata_builder([builder=builder](std::string const& layout, YAML::Node const& data)
    {
        return builder(layout, std::make_unique<YamlDisplayConfigurationNode>(data));
    });
}

miral::DisplayConfiguration::~DisplayConfiguration() = default;

miral::DisplayConfiguration::DisplayConfiguration(miral::DisplayConfiguration const&) = default;

auto miral::DisplayConfiguration::operator=(miral::DisplayConfiguration const&) -> miral::DisplayConfiguration& = default;
