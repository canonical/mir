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

#include "static_display_config.h"

#include <mir/output_type_names.h>
#include <mir/log.h>
#include <mir/main_loop.h>
#include <mir/shell/display_configuration_controller.h>
#include <mir/server.h>
#include "miral/command_line_option.h"
#include "miral/display_configuration.h"
#include "miral/runner.h"

#include "yaml-cpp/yaml.h"
#include <string.h>

#include <sys/inotify.h>
#include <unistd.h>

#include <boost/throw_exception.hpp>

#include <algorithm>
#include <fstream>
#include <optional>
#include <sstream>

namespace mg = mir::graphics;
using namespace mir::geometry;

namespace
{
// A table of known layout strategies that can be used when `display-layout` doesn't match
// the configuration file, and will be added to the generated .display configuration
const struct {
    std::string name;
    std::function<void(mg::DisplayConfiguration& conf)> strategy;
} layout_strategies[] =
    {
        {"default", [](auto& config) { miral::YamlFileDisplayConfig::apply_default_configuration(config); }},
        {"side_by_side", [](auto& config) { mg::SideBySideDisplayConfigurationPolicy{}.apply_to(config); }}
    };

char const* const card_id = "card-id";
char const* const state = "state";
char const* const state_enabled  = "enabled";
char const* const state_disabled = "disabled";
char const* const position = "position";
char const* const mode = "mode";
char const* const orientation = "orientation";
char const* const scale = "scale";
char const* const group = "group";
char const* const orientation_value[] = { "normal", "left", "inverted", "right" };
char const* const layout_suffix = "-layout";

auto as_string(MirOrientation orientation) -> char const*
{
    return orientation_value[orientation/90];
}

auto as_orientation(std::string const& orientation) -> mir::optional_value<MirOrientation>
{
    if (orientation == orientation_value[3])
        return mir_orientation_right;

    if (orientation == orientation_value[2])
        return mir_orientation_inverted;

    if (orientation == orientation_value[1])
        return mir_orientation_left;

    if (orientation == orientation_value[0])
        return mir_orientation_normal;

    return {};
}

auto select_mode_index(size_t mode_index, std::vector<mg::DisplayConfigurationMode> const & modes) -> size_t
{
    if (modes.empty())
        return std::numeric_limits<size_t>::max();

    if (mode_index >= modes.size())
        return 0;

    return mode_index;
}
}

// Ugly hack because Alpine's string.h doesn't declare the basename overloads
#ifndef __GLIBC__
extern "C" { char const* basename(char const*); }
#endif

std::map<miral::YamlFileDisplayConfig::Property, std::string const> const miral::YamlFileDisplayConfig::display_matching_properties()
{
    static std::map<miral::YamlFileDisplayConfig::Property, std::string const> const display_matching_properties{
        {miral::YamlFileDisplayConfig::Property::Vendor, "vendor"},
        {miral::YamlFileDisplayConfig::Property::Model, "model"},
        {miral::YamlFileDisplayConfig::Property::Product, "product"},
        {miral::YamlFileDisplayConfig::Property::Serial, "serial"},
    };
    return display_matching_properties;
};

miral::StaticDisplayConfig::StaticDisplayConfig(std::string const& filename) :
    ReloadingYamlFileDisplayConfig(::basename(filename.c_str()))
{
    config_path(filename.substr(0, filename.size() - basename.size()));
    check_for_layout_override();

    std::ifstream config_file{filename};

    if (config_file)
    {
        load_config(config_file, filename);
    }
    else
    {
        mir::log_info("Cannot read static display configuration file: '" + filename + "'");
    }
}
namespace
{
    auto error_prefix(std::string const& filename)
    {
        return "ERROR: in display configuration file: '" + filename + "' : ";
    }
}

void miral::YamlFileDisplayConfig::parse_configuration(YAML::Node const& node, Config& output_config, std::string const& error_prefix, std::string const& identifier)
{
    if (!node.IsMap())
        throw mir::AbnormalExit{error_prefix + "invalid configuration for " + identifier};

    if (auto const s = node[state])
    {
        auto const state = s.as<std::string>();
        if (state != state_enabled && state != state_disabled)
            throw mir::AbnormalExit{error_prefix + "invalid 'state' (" + state + ") for " + identifier};
        output_config.disabled = (state == state_disabled);
    }

    if (auto const pos = node[position])
    {
        output_config.position = Point{pos[0].as<int>(), pos[1].as<int>()};
    }

    if (auto const group_id = node[group])
    {
        output_config.group_id = group_id.as<int>();
    }

    if (auto const m = node[mode])
    {
        std::istringstream in{m.as<std::string>()};

        char delimiter = '\0';
        int width;
        int height;

        if (!(in >> width))
            goto mode_error;

        if (!(in >> delimiter) || delimiter != 'x')
            goto mode_error;

        if (!(in >> height))
            goto mode_error;

        output_config.size = Size{width, height};

        if (in.peek() == '@')
        {
            double refresh;
            if (!(in >> delimiter) || delimiter != '@')
                goto mode_error;

            if (!(in >> refresh))
                goto mode_error;

            output_config.refresh = refresh;
        }
    }
    mode_error: // TODO better error handling

    if (auto const o = node[orientation])
    {
        std::string const orientation = o.as<std::string>();
        output_config.orientation = as_orientation(orientation);

        if (!output_config.orientation)
            throw mir::AbnormalExit{error_prefix + "invalid 'orientation' (" +
                                    orientation + ") for " + identifier};
    }

    if (auto const s = node[scale])
    {
        output_config.scale = s.as<float>();
    }

    for (auto const& key : custom_output_attributes)
    {
        if (auto const value = node[key])
        {
            output_config.custom_attribute[key] = value.Scalar();
        }
        else
        {
            output_config.custom_attribute[key] = std::nullopt;
        }
    }
}

void miral::YamlFileDisplayConfig::load_config(std::istream& config_file, std::string const& filename)
try
{
    decltype(config) new_config;

    using namespace YAML;
    using std::begin;
    using std::end;

    auto const parsed_config = Load(config_file);
    if (!parsed_config.IsDefined() || !parsed_config.IsMap())
        throw mir::AbnormalExit{error_prefix(filename) + "unrecognized content"};

    Node layouts = parsed_config["layouts"];
    if (!layouts.IsDefined() || !layouts.IsMap())
        throw mir::AbnormalExit{error_prefix(filename) + "no layouts"};

    for (auto const& ll : layouts)
    {
        auto const& layout_name = ll.first.as<std::string>();
        auto const& layout = ll.second;

        if (!layout.IsDefined() || !layout.IsMap())
        {
            throw mir::AbnormalExit{error_prefix(filename) + "invalid '" + layout_name + "' layout"};
        }

        Matchers2Config layout_config;

        Node cards = layout["cards"];
        Node displays = layout["displays"];

        if (!cards.IsDefined() && !displays.IsDefined())
            throw mir::AbnormalExit{error_prefix(filename) + "neither 'cards' or 'displays' defined in '" + layout_name + "' layout"};

        if (cards.IsDefined() && !cards.IsSequence())
            throw mir::AbnormalExit{error_prefix(filename) + "invalid 'cards' in '" + layout_name + "' layout"};

        if (displays.IsDefined())
        {
            if(!displays.IsSequence())
            {
                throw mir::AbnormalExit{error_prefix(filename) + "invalid 'displays' in '" + layout_name + "' layout"};
            }

            for (size_t n = 0; n < displays.size(); n++)
            {
                auto const identifier = "displays['" + std::to_string(n) + "']";
                auto const& display = displays[n];
                if (!display.IsDefined() || !display.IsMap() || !display.size())
                    throw mir::AbnormalExit{error_prefix(filename) + "invalid '" + identifier + "' in '" + layout_name + "' layout"};

                Matchers matchers;

                for (auto const& [property, key] : display_matching_properties())
                {
                    auto const value = display[key];
                    if (value.IsDefined() && (!value.IsScalar() || value.as<std::string>().empty()))
                    {
                        throw mir::AbnormalExit{error_prefix(filename) + "invalid '" + key + "' in '" + identifier + "' in '" + layout_name + "' layout"};
                    }
                    else if (value)
                    {
                        matchers[property] = value.as<std::string>();
                    }
                }

                Config output_config;
                parse_configuration(display, output_config, error_prefix(filename), identifier);
                layout_config.emplace_back(matchers, output_config);
            }
        }

        for (Node const& card : cards)
        {
            mg::DisplayConfigurationCardId card_no;

            if (auto const id = card[card_id])
            {
                card_no = mg::DisplayConfigurationCardId{id.as<int>()};
            }

            if (!card.IsDefined() || !(card.IsMap() || card.IsNull()))
                throw mir::AbnormalExit{error_prefix(filename) + "invalid card: " + std::to_string(card_no.as_value())};

            for (std::pair<Node, Node> port : card)
            {
                auto const port_name = port.first.Scalar();

                if (port_name == card_id)
                    continue;

                auto const& port_config = port.second;

                if (port_config.IsDefined() && !port_config.IsNull())
                {
                    Config output_config;
                    parse_configuration(port_config, output_config, error_prefix(filename), port_name);
                    Matchers const port_matcher{{Property::Port, port_name}};
                    layout_config.emplace_back(port_matcher, output_config);
                }
            }
        }

        // Users may provide a function that returns a custom user data on a layout
        // based off of its configuration in YAML.
        std::map<std::string, std::any> userdata_map;
        for (auto const& [key, builder] : layout_userdata_builder_funcs)
        {
            if (layout[key])
                userdata_map[key] = builder(layout[key]);
        }

        new_config[ll.first.Scalar()] = { layout_config, userdata_map };
    }

    std::lock_guard lock{mutex};
    config = new_config;
    mir::log_debug("Loaded display configuration file: %s", filename.c_str());
}
catch (YAML::Exception const& x)
{
    throw mir::AbnormalExit{error_prefix(filename) + x.what()};
}

void miral::YamlFileDisplayConfig::apply_to(mg::DisplayConfiguration& conf)
{
    auto const i = std::find_if(std::begin(layout_strategies), std::end(layout_strategies),
                                [layout=layout](auto strategy) { return strategy.name == layout; });

    if (i != std::end(layout_strategies))
    {
        i->strategy(conf);
    }

    std::lock_guard lock{mutex};
    auto const current_config = config.find(layout);

    if (current_config != end(config))
    {
        mir::log_debug("Display config using layout: '%s'", layout.c_str());

        conf.for_each_output([&config=current_config->second.matchers2config](mg::UserDisplayConfigurationOutput& conf_output)
            {
                for (auto const& [matchers, conf] : config)
                {
                    for (auto const& [property, value] : matchers)
                    {
                        static auto const props = StaticDisplayConfig::display_matching_properties();

                        switch (property)
                        {
                            case Property::Port:
                                if (conf_output.name != value)
                                {
                                    goto failed_match;
                                }
                                break;

                            case Property::Vendor:
                                if (auto const vendor = conf_output.display_info.vendor; !vendor || vendor.value() != value)
                                {
                                    goto failed_match;
                                }
                                break;

                            case Property::Model:
                                if (auto const model = conf_output.display_info.model; !model || model.value() != value)
                                {
                                    goto failed_match;
                                }
                                break;

                            case Property::Product:
                                if (auto const product_code = conf_output.display_info.product_code; !product_code || std::to_string(product_code.value()) != value)
                                {
                                    goto failed_match;
                                }
                                break;

                            case Property::Serial:
                                if (auto const serial = conf_output.display_info.serial; !serial || serial.value() != value)
                                {
                                    goto failed_match;
                                }
                                break;
                        }
                    }

                    // If we get here, all matchers applied to this output
                    apply_to_output(conf_output, conf);
                    return;

                    // But here we failed to match
                    failed_match:;
                }
            });
    }
    else
    {
        conf.for_each_output([this](mg::UserDisplayConfigurationOutput& conf_output)
            {
                for (auto const& key : custom_output_attributes)
                {
                    conf_output.custom_attribute[key] = std::nullopt;
                }
            });

        if (i != std::end(layout_strategies))
        {
            mir::log_debug("Display config using layout strategy: '%s'", layout.c_str());
        }
        else
        {
            mir::log_warning("Display config does not contain layout '%s'", layout.c_str());
            mir::log_debug("Display config using layout strategy: 'default'");
            apply_default_configuration(conf);
        }
    }

    std::ostringstream out;
    out << "Display config:\n8>< ---------------------------------------------------\n";
    out << "layouts:"
           "\n  " << layout << ":                         # the current layout";

    serialize_configuration(out, conf);
    out << "8>< ---------------------------------------------------";
    mir::log_info(out.str());
}

void miral::YamlFileDisplayConfig::confirm(mir::graphics::DisplayConfiguration const&)
{
}

void miral::YamlFileDisplayConfig::apply_default_configuration(mg::DisplayConfiguration& conf)
{
    conf.for_each_output([config=Config{}](mg::UserDisplayConfigurationOutput& conf_output)
        {
            apply_to_output(conf_output, config);
        });
}

void miral::YamlFileDisplayConfig::serialize_configuration(std::ostream& out, mg::DisplayConfiguration& conf)
{
    out << "\n    cards:"
           "\n    # a list of cards (currently matched by card-id)";

    std::map<mg::DisplayConfigurationCardId, std::ostringstream> card_map;
    std::ostringstream displays;

    conf.for_each_output([&card_map, &displays](mg::UserDisplayConfigurationOutput const& conf_output)
        {
            serialize_output_configuration(card_map[conf_output.card_id], conf_output);
            serialize_display_info(displays, conf_output);
        });

    for (auto const& co : card_map)
    {
        out << "\n"
               "\n    - card-id: " << co.first.as_value()
            << co.second.str();
    }

    out << "\n"
           "\n    # displays:"
           "\n    # A list of display configurations matched by the displays' properties"
           "\n    #"
           "\n    # These take the same display options as above,"
           "\n    # and take precedence over the port-based configuration."
           "\n    #";

    if (displays.str().empty())
    {
        out << "\n    # No display properties could be determined."
               "\n";
    }
    else
    {
        out << displays.str() << "\n";
    }
}

void miral::YamlFileDisplayConfig::serialize_output_configuration(
    std::ostream& out, mg::UserDisplayConfigurationOutput const& conf_output)
{
    out << "\n      " << conf_output.name << ':';

    if (conf_output.connected && conf_output.modes.size() > 0)
    {
        out << "\n        # This output supports the following modes:";
        for (size_t i = 0; i < conf_output.modes.size(); ++i)
        {
            if (i) out << ',';
            if ((i % 5) != 2) out << ' ';
            else out << "\n        # ";
            out << conf_output.modes[i];
        }
        out << "\n        #"
               "\n        # Uncomment the following to enforce the selected configuration."
               "\n        # Or amend as desired."
               "\n        #"
               "\n        state: " << (conf_output.used ? state_enabled : state_disabled)
            << "\t# {enabled, disabled}, defaults to enabled";

        if (conf_output.used) // The following are only set when used
        {
            out << "\n        mode: " << conf_output.modes[conf_output.current_mode_index]
                << "\t# Defaults to preferred mode"
                   "\n        position: [" << conf_output.top_left.x << ", " << conf_output.top_left.y << ']'
                << "\t# Defaults to [0, 0]"
                   "\n        orientation: " << as_string(conf_output.orientation)
                << "\t# {normal, left, right, inverted}, defaults to normal"
                   "\n        scale: " << conf_output.scale
                << "\n        group: " << conf_output.logical_group_id.as_value()
                << "\t# Outputs with the same non-zero value are treated as a single display";
        }

        for (auto const& [key, value] : conf_output.custom_attribute)
        {
            if (value)
            {
                out << "\n        " << key << ": " << value.value();
            }
        }
    }
    else
    {
        out << "\n        # (disconnected)";
    }

    out << "\n";
}

void miral::YamlFileDisplayConfig::serialize_display_info(
    std::ostream& out, mg::UserDisplayConfigurationOutput const& conf_output)
{
    out << "\n    # - vendor: " << conf_output.display_info.vendor.value_or("")
        << "\n    #   model: " << conf_output.display_info.model.value_or("")
        << "\n    #   product: " << conf_output.display_info.product_code.value_or(0)
        << "\n    #   serial: " << conf_output.display_info.serial.value_or("")
        << "\n    #";
}

void miral::YamlFileDisplayConfig::apply_to_output(mg::UserDisplayConfigurationOutput& conf_output, Config const& conf)
{
    if (conf_output.connected && conf_output.modes.size() > 0 && !conf.disabled)
    {
        conf_output.used = true;
        conf_output.power_mode = mir_power_mode_on;
        conf_output.orientation = mir_orientation_normal;

        if (conf.position.is_set())
        {
            conf_output.top_left = conf.position.value();
        }
        else
        {
            conf_output.top_left = Point{0, 0};
        }

        size_t preferred_mode_index{select_mode_index(conf_output.preferred_mode_index, conf_output.modes)};
        conf_output.current_mode_index = preferred_mode_index;

        if (conf.size.is_set())
        {
            bool matched_mode = false;

            for (auto mode = begin(conf_output.modes); mode != end(conf_output.modes); ++mode)
            {
                if (mode->size == conf.size.value())
                {
                    if (conf.refresh.is_set())
                    {
                        if (std::abs(conf.refresh.value() - mode->vrefresh_hz) < 1.0)
                        {
                            conf_output.current_mode_index = distance(begin(conf_output.modes), mode);
                            matched_mode = true;
                        }
                    }
                    else if (conf_output.modes[conf_output.current_mode_index].size != conf.size.value()
                          || conf_output.modes[conf_output.current_mode_index].vrefresh_hz < mode->vrefresh_hz)
                    {
                        conf_output.current_mode_index = distance(begin(conf_output.modes), mode);
                        matched_mode = true;
                    }
                }
            }

            if (!matched_mode)
            {
                if (conf.refresh.is_set())
                {
                    mir::log_warning("Display config contains unmatched mode: '%dx%d@%2.1f'",
                        conf.size.value().width.as_int(), conf.size.value().height.as_int(), conf.refresh.value());
                }
                else
                {
                    mir::log_warning("Display config contains unmatched mode: '%dx%d'",
                                     conf.size.value().width.as_int(), conf.size.value().height.as_int());
                }
            }
        }

        if (conf.scale.is_set())
        {
            conf_output.scale = conf.scale.value();
        }

        if (conf.orientation.is_set())
        {
            conf_output.orientation = conf.orientation.value();
        }

        if (conf.group_id.is_set())
        {
            conf_output.logical_group_id = mg::DisplayConfigurationLogicalGroupId{conf.group_id.value()};
        }
        else
        {
            conf_output.logical_group_id = mg::DisplayConfigurationLogicalGroupId{};
        }
    }
    else
    {
        conf_output.used = false;
        conf_output.power_mode = mir_power_mode_off;
    }

    conf_output.custom_attribute = conf.custom_attribute;
}

void miral::YamlFileDisplayConfig::select_layout(std::string const& layout)
{
    std::lock_guard lock{mutex};
    this->layout = layout;
}

auto miral::YamlFileDisplayConfig::list_layouts() const -> std::vector<std::string>
{
    std::vector<std::string> result;

    std::lock_guard lock{mutex};
    for (auto const& c: config)
    {
        result.push_back(c.first);
    }

    return result;
}

auto miral::YamlFileDisplayConfig::layout_userdata(std::string const& key) -> std::optional<std::any const>
{
    std::lock_guard lock{mutex};
    for (auto const& [first, second]: config)
    {
        if (first == layout)
        {
            if (second.userdata.contains(key))
                return second.userdata.at(key);

            mir::log_info("Parsing display configuration: No user data on layout=%s for key=%s", layout.c_str(), key.c_str());
            return std::nullopt;
        }
    }

    mir::log_warning("Parsing display configuration: Cannot find layout: %s", layout.c_str());
    return std::nullopt;
}

void miral::YamlFileDisplayConfig::add_output_attribute(std::string const& key)
{
    custom_output_attributes.insert(key);
}

void miral::YamlFileDisplayConfig::layout_userdata_builder(
    std::string const& key,
    std::function<std::any(YAML::Node const&)> const& builder)
{
    layout_userdata_builder_funcs[key] = builder;
}

miral::ReloadingYamlFileDisplayConfig::ReloadingYamlFileDisplayConfig(std::string basename) :
    basename{std::move(basename)}
{
}

miral::ReloadingYamlFileDisplayConfig::~ReloadingYamlFileDisplayConfig()
{
    if (auto const ml = the_main_loop())
    {
        ml->unregister_fd_handler(this);
    }
}

void miral::ReloadingYamlFileDisplayConfig::init_auto_reload(mir::Server& server)
{
    {
        std::lock_guard lock{mutex};
        if (the_display_configuration_controller_.use_count() || the_main_loop_.use_count())
        {
            mir::fatal_error("Init function should only be called once: %s", __PRETTY_FUNCTION__);
        }

        the_display_configuration_controller_ = server.the_display_configuration_controller();
        the_main_loop_ = server.the_main_loop();
    }
    auto_reload();
}

void miral::ReloadingYamlFileDisplayConfig::config_path(std::string newpath)
{
    {
        std::lock_guard lock{mutex};
        config_path_ = newpath;
    }

    check_for_layout_override();
}

void miral::ReloadingYamlFileDisplayConfig::confirm(mir::graphics::DisplayConfiguration const& conf)
{
    std::lock_guard lock{mutex};
    if (!config_path_)
    {
        mir::log_debug("Nowhere to write display configuration template: Neither XDG_CONFIG_HOME or HOME is set");
    }
    else
    {
        auto const filename = config_path_.value() + "/" + basename;

        if (access(filename.c_str(), F_OK))
        {
            std::ofstream out{filename};

            out << "layouts:"
                   "\n# keys here are layout labels (used for atomically switching between them)."
                   "\n# The yaml anchor 'the_default' is used to alias the 'default' label"
                   "\n";

            for (auto const& strategy : layout_strategies)
            {
                auto const resulting_layout = conf.clone();
                strategy.strategy(*resulting_layout);

                out << "\n  " << strategy.name << ":";
                serialize_configuration(out, *resulting_layout);
            }

            mir::log_debug(
                "%s display configuration template: %s",
                out ? "Wrote" : "Failed writing",
                filename.c_str());
        }
    }
}

auto miral::ReloadingYamlFileDisplayConfig::the_main_loop() const -> std::shared_ptr<mir::MainLoop>
{
    std::lock_guard lock{mutex};
    return the_main_loop_.lock();
}

auto miral::ReloadingYamlFileDisplayConfig::the_display_configuration_controller() const -> std::shared_ptr<mir::shell::DisplayConfigurationController>
{
    std::lock_guard lock{mutex};
    return the_display_configuration_controller_.lock();
}

auto miral::ReloadingYamlFileDisplayConfig::inotify_config_path() -> std::optional<mir::Fd>
{
    std::lock_guard lock{mutex};

    if (!config_path_) return std::nullopt;

    mir::Fd inotify_fd{inotify_init1(IN_CLOEXEC)};

    if (inotify_fd < 0)
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to initialize inotify_fd"}));

    config_path_wd = inotify_add_watch(inotify_fd, config_path_.value().c_str(), IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO);

    return inotify_fd;
}

void miral::ReloadingYamlFileDisplayConfig::auto_reload()
{
    if (auto const icf = inotify_config_path())
    {
        if (auto const ml = the_main_loop())
        {
            ml->register_fd_handler({icf.value()}, this, [icf=icf.value(), this] (int)
            {
                static size_t const sizeof_inotify_event = sizeof(inotify_event);

                alignas(inotify_event) char buffer[sizeof(inotify_event) + NAME_MAX + 1];

                auto const readsize = read(icf, buffer, sizeof(buffer));
                if (readsize < static_cast<ssize_t>(sizeof_inotify_event))
                    return;

                auto raw_buffer = buffer;
                while (raw_buffer != buffer + readsize)
                {
                    // This is safe because buffer is aligned and event.len includes padding for alignment
                    auto& event = reinterpret_cast<inotify_event&>(*raw_buffer);
                    if (event.mask & (IN_CLOSE_WRITE | IN_MOVED_TO))
                    try
                    {
                        if (event.name == basename)
                        {
                            std::lock_guard lock{mutex};
                            auto const& filename = config_path_.value() + "/" + basename;

                            if (std::ifstream config_file{filename})
                            {
                                load_config(config_file, filename);
                            }
                            else
                            {
                                mir::log_warning("Failed to open display configuration: %s", filename.c_str());
                            }
                        }
                        else if (event.name == basename + layout_suffix)
                        {
                            check_for_layout_override();
                        }
                        else
                        {
                            return;
                        }

                        if (auto const dcc = the_display_configuration_controller())
                        {
                            auto config = dcc->base_configuration();
                            apply_to(*config);
                            dcc->set_base_configuration(config);
                        }
                    }
                    catch (mir::AbnormalExit const& except)
                    {
                        mir::log_warning("Failed to reload display configuration: %s", except.what());
                    }

                    raw_buffer += sizeof_inotify_event+event.len;
                }
            });
        }
    }
}

void miral::ReloadingYamlFileDisplayConfig::check_for_layout_override()
{
    std::lock_guard lock{mutex};
    if (!config_path_) return;

    if (std::ifstream layout_file{config_path_.value() + "/" + basename + layout_suffix})
    {
        std::string new_layout;
        layout_file >> new_layout;
        select_layout(new_layout);
    }
}