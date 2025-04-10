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

void miral::DisplayConfiguration::add_output_attribute(std::string const& key)
{
    self->add_output_attribute(key);
}

auto miral::DisplayConfiguration::layout_userdata(std::string const& key) const
    -> std::optional<std::any const>
{
    return self->layout_userdata(key);
}

struct miral::DisplayConfiguration::Node::Self
{
    explicit Self(YAML::Node const& node)
        : node(node)
    {
    }

    ~Self() = default;

    template <typename T>
    auto as() const -> T
    {
        try
        {
            return node.as<T>();
        }
        catch (YAML::Exception const&)
        {
            return T();
        }
    }

    YAML::Node const node;
};

miral::DisplayConfiguration::Node::Node(std::unique_ptr<Self> self)
    : self(std::move(self))
{
}

miral::DisplayConfiguration::Node::~Node() = default;

auto miral::DisplayConfiguration::Node::type() const -> Type
{
    if (self->node.IsScalar())
    {
        try {
            (void)self->node.as<int>();
            return Type::integer;
        } catch (const YAML::BadConversion& e) {
            return Type::string;
        }
    }
    else if (self->node.IsMap())
        return Type::map;
    else if (self->node.IsSequence())
        return Type::sequence;
    else
    {
        mir::log_error("Checked type is none of the supported types");
        return Type::unknown;
    }
}

auto miral::DisplayConfiguration::Node::as_string() const -> std::string
{
    if (type() != Type::string)
        mir::fatal_error("Attempting to access a Node of type %d as a string", type());

    return self->as<std::string>();
}

auto miral::DisplayConfiguration::Node::as_int() const -> int
{
    if (type() != Type::integer)
        mir::fatal_error("Attempting to access a Node of type %d as an integer", type());

    return self->as<int>();
}

void miral::DisplayConfiguration::Node::for_each(std::function<void(Node const&)> const& f) const
{
    if (type() != Type::sequence)
        mir::fatal_error("Attempting to access a Node of type as a sequence", type());

    for (auto const& item : self->node)
        f(Node(std::make_unique<Self>(item)));
}

auto miral::DisplayConfiguration::Node::has(std::string const& key) const -> bool
{
    if (type() != Type::map)
        mir::fatal_error("Attempting to access a Node of type as a map", type());

    return self->node[key].operator bool();
}

auto miral::DisplayConfiguration::Node::at(std::string const& key) const -> Node
{
    if (!has(key))
        mir::fatal_error("Attempting to access key that does not exist in the map: %s is missing", key.c_str());

    return Node(std::make_unique<Self>(self->node[key]));
}

void miral::DisplayConfiguration::layout_userdata_builder(
    std::string const& key,
    std::function<std::any(Node const& value)> const& builder) const
{
    self->layout_userdata_builder(key, [builder=builder](YAML::Node const& data)
    {
        return builder(Node(std::make_unique<Node::Self>(data)));
    });
}

miral::DisplayConfiguration::~DisplayConfiguration() = default;

miral::DisplayConfiguration::DisplayConfiguration(miral::DisplayConfiguration const&) = default;

auto miral::DisplayConfiguration::operator=(miral::DisplayConfiguration const&) -> miral::DisplayConfiguration& = default;
