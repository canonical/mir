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

#include <optional>
#include <mutex>
#include <boost/throw_exception.hpp>
#include <sys/inotify.h>
#include <unistd.h>
#include <mir/main_loop.h>
#include <mir/shell/display_configuration_controller.h>
#include <mir/server.h>
#include "miral/command_line_option.h"
#include "miral/runner.h"
#include "miral/display_configuration.h"
#include "static_display_config.h"

#include <mir/output_type_names.h>

#include <mir/log.h>


#include "yaml-cpp/yaml.h"
#include <string.h>
#include <libgen.h>
#undef basename

#include <algorithm>
#include <fstream>
#include <sstream>

namespace mg = mir::graphics;
using namespace mir::geometry;

namespace
{
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

auto output_type_from(std::string const& output) -> MirOutputType
{
    for (int i = 0; auto const name = mir::output_type_name(static_cast<MirOutputType>(i)); ++i)
    {
        if (output.find(name) == 0)
            return static_cast<MirOutputType>(i);
    }
    return mir_output_type_unknown;
}

auto output_index_from(std::string const& output) -> int
{
    std::istringstream in{output.substr(output.rfind('-')+1)};
    int result = 0;
    in >> result;
    return result;
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

miral::StaticDisplayConfig::StaticDisplayConfig(std::string const& filename) :
    ReloadingYamlFileDisplayConfig(::basename(filename.c_str()))
{
    config_path(filename.substr(0, filename.size() - basename.size()));

    std::ifstream config_file{filename};

    if (config_file)
    {
        load_config(config_file, "ERROR: in display configuration file: '" + filename + "' : ");
    }
    else
    {
        mir::log_info("Cannot read static display configuration file: '" + filename + "'");
    }
}

void miral::YamlFileDisplayConfig::load_config(std::istream& config_file, std::string const& error_prefix)
try
{
    decltype(config) new_config;

    using namespace YAML;
    using std::begin;
    using std::end;

    auto const parsed_config = Load(config_file);
    if (!parsed_config.IsDefined() || !parsed_config.IsMap())
        throw mir::AbnormalExit{error_prefix + "unrecognized content"};

    Node layouts = parsed_config["layouts"];
    if (!layouts.IsDefined() || !layouts.IsMap())
        throw mir::AbnormalExit{error_prefix + "no layouts"};

    for (auto const& ll : layouts)
    {
        auto const& layout = ll.second;

        if (!layout.IsDefined() || !layout.IsMap())
        {
            throw mir::AbnormalExit{error_prefix + "invalid '" + ll.first.as<std::string>() + "' layout"};
        }

        Id2Config layout_config;

        Node cards = layout["cards"];

        if (!cards.IsDefined() || !cards.IsSequence())
            throw mir::AbnormalExit{error_prefix + "invalid 'cards' in '" + ll.first.as<std::string>() + "' layout"};

        for (Node const& card : cards)
        {
            mg::DisplayConfigurationCardId card_no;

            if (auto const id = card[card_id])
            {
                card_no = mg::DisplayConfigurationCardId{id.as<int>()};
            }

            if (!card.IsDefined() || !(card.IsMap() || card.IsNull()))
                throw mir::AbnormalExit{error_prefix + "invalid card: " + std::to_string(card_no.as_value())};

            for (std::pair<Node, Node> port : card)
            {
                auto const port_name = port.first.Scalar();

                if (port_name == card_id)
                    continue;

                auto const& port_config = port.second;

                if (port_config.IsDefined() && !port_config.IsNull())
                {
                    if (!port_config.IsMap())
                        throw mir::AbnormalExit{error_prefix + "invalid port: " + port_name};

                    Id const output_id{card_no, output_type_from(port_name), output_index_from(port_name)};
                    Config   output_config;

                    if (auto const s = port_config[state])
                    {
                        auto const state = s.as<std::string>();
                        if (state != state_enabled && state != state_disabled)
                            throw mir::AbnormalExit{error_prefix + "invalid 'state' (" + state + ") for port: " + port_name};
                        output_config.disabled = (state == state_disabled);
                    }

                    if (auto const pos = port_config[position])
                    {
                        output_config.position = Point{pos[0].as<int>(), pos[1].as<int>()};
                    }

                    if (auto const group_id = port_config[group])
                    {
                        output_config.group_id = group_id.as<int>();
                    }

                    if (auto const m = port_config[mode])
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

                    if (auto const o = port_config[orientation])
                    {
                        std::string const orientation = o.as<std::string>();
                        output_config.orientation = as_orientation(orientation);

                        if (!output_config.orientation)
                            throw mir::AbnormalExit{error_prefix + "invalid 'orientation' (" +
                                                    orientation + ") for port: " + port_name};
                    }

                    if (auto const s = port_config[scale])
                    {
                        output_config.scale = s.as<float>();
                    }

                    layout_config[output_id] = output_config;
                }
            }
        }
        new_config[ll.first.Scalar()] = layout_config;
    }

    config = new_config;
}
catch (YAML::Exception const& x)
{
    throw mir::AbnormalExit{error_prefix + x.what()};
}

void miral::YamlFileDisplayConfig::apply_to(mg::DisplayConfiguration& conf)
{
    auto const current_config = config.find(layout);

    if (current_config != end(config))
    {
        mir::log_debug("Display config using layout: '%s'", layout.c_str());
    }
    else
    {
        mir::log_warning("Display config does not contain layout '%s'", layout.c_str());
    }

    struct card_data
    {
        std::ostringstream out;
        std::map<MirOutputType, int> output_counts;
    };
    std::map<mg::DisplayConfigurationCardId, card_data> card_map;

    conf.for_each_output([&](mg::UserDisplayConfigurationOutput& conf_output)
        {
            auto& card_data = card_map[conf_output.card_id];
            auto& out = card_data.out;
            auto const type = static_cast<MirOutputType>(conf_output.type);
            auto const index_by_type = ++card_data.output_counts[type];

            auto const& conf = (current_config != end(config)) ?
                         current_config->second[Id{conf_output.card_id, type, index_by_type}] :
                          Config{};

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

            out << "\n      " << mir::output_type_name(type);
            if (conf_output.card_id.as_value() > 0)
                out << '-' << conf_output.card_id.as_value();
            out << '-' << index_by_type << ':';

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
                       "\n        # state: " << (conf_output.used ? state_enabled : state_disabled)
                    << "\t# {enabled, disabled}, defaults to enabled";

                if (conf_output.used) // The following are only set when used
                {
                    out << "\n        # mode: " << conf_output.modes[conf_output.current_mode_index]
                        << "\t# Defaults to preferred mode"
                           "\n        # position: [" << conf_output.top_left.x << ", " << conf_output.top_left.y << ']'
                        << "\t# Defaults to [0, 0]"
                           "\n        # orientation: " << as_string(conf_output.orientation)
                        << "\t# {normal, left, right, inverted}, defaults to normal"
                           "\n        # scale: " << conf_output.scale
                        << "\n        # group: " << conf_output.logical_group_id.as_value()
                        << "\t# Outputs with the same non-zero value are treated as a single display";
                }
            }
            else
            {
                out << "\n        # (disconnected)";
            }

            out << "\n";
        });

    auto print_template_config = [&card_map](std::ostream& out)
        {
            out << "layouts:"
                   "\n# keys here are layout labels (used for atomically switching between them)"
                   "\n# when enabling displays, surfaces should be matched in reverse recency order"
                   "\n"
                   "\n  default:                         # the default layout"
                   "\n"
                   "\n    cards:"
                   "\n    # a list of cards (currently matched by card-id)";

            for (auto const& co : card_map)
            {
                out << "\n"
                       "\n    - card-id: " << co.first.as_value()
                    << co.second.out.str();
            }
        };

    dump_config(print_template_config);
}

void miral::YamlFileDisplayConfig::dump_config(std::function<void(std::ostream&)> const& print_template_config)
{
    std::ostringstream out;
    out << "Display config:\n8>< ---------------------------------------------------\n";
    print_template_config(out);
    out << "8>< ---------------------------------------------------";
    mir::log_info(out.str());
}

void miral::YamlFileDisplayConfig::select_layout(std::string const& layout)
{
    this->layout = layout;
}

auto miral::YamlFileDisplayConfig::list_layouts() const -> std::vector<std::string>
{
    std::vector<std::string> result;

    for (auto const& c: config)
    {
        result.push_back(c.first);
    }

    return result;
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
        the_display_configuration_controller_ = server.the_display_configuration_controller();
        the_main_loop_ = server.the_main_loop();
    }
    auto_reload();
}

void miral::ReloadingYamlFileDisplayConfig::config_path(std::string newpath)
{
    std::lock_guard lock{mutex};
    config_path_ = newpath;
}

void miral::ReloadingYamlFileDisplayConfig::dump_config(std::function<void(std::ostream&)> const& print_template_config)
{
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
            print_template_config(out);

            mir::log_debug(
                "%s display configuration template: %s",
                out ? "Written" : "Failed writing",
                filename.c_str());
        }
    }

    YamlFileDisplayConfig::dump_config(print_template_config);
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

    mir::Fd inotify_fd{inotify_init()};

    if (inotify_fd < 0)
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to initialize inotify_fd"}));

    config_path_wd = inotify_add_watch(inotify_fd, config_path_.value().c_str(), IN_CLOSE_WRITE | IN_CREATE | IN_DELETE);

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
                union
                {
                    inotify_event event;
                    char buffer[sizeof(inotify_event) + NAME_MAX + 1];
                } inotify_buffer;

                if (read(icf, &inotify_buffer, sizeof(inotify_buffer)) < static_cast<ssize_t>(sizeof(inotify_event)))
                    return;

                if (inotify_buffer.event.mask & (IN_CLOSE_WRITE | IN_CREATE)
                    && inotify_buffer.event.name == basename)
                    try
                    {
                        auto const& filename = config_path_.value() + "/" + basename;

                        if (std::ifstream config_file{filename})
                        {
                            load_config(config_file, "ERROR: in display configuration file: '" + filename + "' : ");

                            if (auto const dcc = the_display_configuration_controller())
                            {
                                auto config = dcc->base_configuration();
                                apply_to(*config);
                                dcc->set_base_configuration(config);
                            }
                        }
                        else
                        {
                            mir::log_warning("Failed to open display configuration: %s", filename.c_str());
                        }
                    }
                    catch (mir::AbnormalExit const& except)
                    {
                        mir::log_warning("Failed to reload display configuration: %s", except.what());
                    }
                });
        }
    }
}
