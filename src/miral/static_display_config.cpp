/*
 * Copyright © 2018 Canonical Ltd.
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

#include "static_display_config.h"

#include <mir_toolkit/mir_client_library.h>

#include <mir/log.h>


#include "yaml-cpp/yaml.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

namespace mg = mir::graphics;
using namespace mir::geometry;

namespace
{
constexpr char const* const output_id = "output_id";
constexpr char const* const position = "position";
constexpr char const* const mode = "mode";
constexpr char const* const orientation = "orientation";


constexpr char const* const orientation_value[] =
    { "normal", "left", "inverted", "right" };

auto as_string(MirOrientation orientation) -> char const*
{
    return orientation_value[orientation/90];
}

auto as_orientation(std::string const& orientation) -> MirOrientation
{
    if (orientation == orientation_value[3])
        return mir_orientation_right;

    if (orientation == orientation_value[2])
        return mir_orientation_inverted;

    if (orientation == orientation_value[1])
        return mir_orientation_left;

    return mir_orientation_normal;
}
}

size_t select_mode_index(size_t mode_index, std::vector<mg::DisplayConfigurationMode> const & modes)
{
    if (modes.empty())
        return std::numeric_limits<size_t>::max();

    if (mode_index >= modes.size())
        return 0;

    return mode_index;
}

miral::StaticDisplayConfig::StaticDisplayConfig(std::string const& filename)
try
{
    using namespace YAML;
    using std::begin;
    using std::end;

    std::ifstream config_file{filename};

    if (!config_file)
    {
        mir::log_warning("Cannot read static display configuration file: '" + filename + "'");
        return;
    }

    auto const config = Load(config_file);
    if (!config.IsDefined() || !config.IsMap())
        throw mir::AbnormalExit{"ERROR: in display configuration file: '" + filename + "' : unrecognized content"};

    Node layouts = config["layouts"];
    if (!layouts.IsDefined() || !layouts.IsMap())
        throw mir::AbnormalExit{"ERROR: in display configuration file: '" + filename + "' : no layouts"};

    auto const layout = begin(config["layouts"]);

    if (layout != end(config["layouts"]))
    {
        Node name_node;
        Node config_node;

        std::tie(name_node, config_node) = *layout;

        if (!config_node.IsDefined() || !config_node.IsMap())
            throw mir::AbnormalExit{"ERROR: in display configuration file: '" + filename +
                                    "' : invalid layout: " + name_node.Scalar()};

        Node cards = config_node["displays"];

        if (!cards.IsDefined() || !cards.IsSequence())
            throw mir::AbnormalExit{"ERROR: in display configuration file: '" + filename +
                                    "' : invalid 'displays' in " + name_node.Scalar()};

        for (Node const card : cards)
        {
            int card_no = 0;

            if (auto const id = card["card-id"])
            {
                card_no = id.as<int>();
            }

            if (!card.IsDefined() || !(card.IsMap() || card.IsNull()))
                throw mir::AbnormalExit{"ERROR: in display configuration file: '" + filename +
                                        "' : invalid card: " + std::to_string(card_no)};

            for (std::pair<Node, Node> port : card)
            {
                auto const port_name = port.first.Scalar();

                if (port_name == "card-id")
                    continue;

                auto const& port_config = port.second;

                if (port_config.IsDefined())
                {
                    if (!port_config.IsDefined() || !port_config.IsMap())
                        throw mir::AbnormalExit{"ERROR: in display configuration file: '" + filename +
                                                "' : invalid card: " + std::to_string(card_no)};

                    // TODO remove requirement for port field
                    if (auto const p = port_config["port"])
                    {
                        int const port_no = p.as<int>();

                        Id const output_id{mg::DisplayConfigurationCardId{card_no}, mg::DisplayConfigurationOutputId{port_no}};
                        Config   output_config;

                        if (auto const pos = port_config[position])
                        {
                            output_config.position = Point{pos[0].as<int>(), pos[1].as<int>()};
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
                        }

                        this->config[output_id] = output_config;
                    }
                }
            }
        }
    }
}
catch (YAML::Exception const& x)
{
    throw mir::AbnormalExit{"ERROR: in display configuration file: '" + filename + "' : " + x.what()};
}

void miral::StaticDisplayConfig::apply_to(mg::DisplayConfiguration& conf)
{
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

            if (conf_output.connected && conf_output.modes.size() > 0)
            {
                conf_output.used = true;
                conf_output.power_mode = mir_power_mode_on;
                conf_output.orientation = mir_orientation_normal;

                auto& conf = config[Id{conf_output.card_id, conf_output.id}];

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
                    for (auto mode = begin(conf_output.modes); mode != end(conf_output.modes); ++mode)
                    {
                        if (mode->size == conf.size.value())
                        {
                            if (conf.refresh.is_set())
                            {
                                if (abs(conf.refresh.value() - mode->vrefresh_hz) < 1.0)
                                {
                                    conf_output.current_mode_index = distance(begin(conf_output.modes), mode);
                                }
                            }
                            else if (conf_output.modes[conf_output.current_mode_index].size != conf.size.value()
                                  || conf_output.modes[conf_output.current_mode_index].vrefresh_hz < mode->vrefresh_hz)
                            {
                                conf_output.current_mode_index = distance(begin(conf_output.modes), mode);
                            }
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
            }
            else
            {
                conf_output.used = false;
                conf_output.power_mode = mir_power_mode_off;
            }

            auto const type = static_cast<MirOutputType>(conf_output.type);

            out << "\n      " << mir_output_type_name(type);
            if (conf_output.card_id.as_value() > 0)
                out << '-' << conf_output.card_id.as_value();
            out << '-' << ++card_data.output_counts[type] << ':';

            out << "\n        port: " << conf_output.id.as_value(); // TODO obsolete

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
                out << "\n        #";
                out << "\n        # Uncomment the following to enforce the selected configuration."
                       "\n        # Or amend as desired.";
                out << "\n        #";
                out << "\n        #mode: " << conf_output.modes[conf_output.current_mode_index]
                    << "\t# Defaults to preferred mode";
                out << "\n        #position: [" << conf_output.top_left.x << ", " << conf_output.top_left.y << ']'
                    << "\t# Defaults to [0, 0]";
                out << "\n        #orientation: " << as_string(conf_output.orientation)
                    << "\t# {normal, left, right, inverted}, Defaults to normal";
            }
            else
            {
                out << "\n        # (disconnected)";
            }

            out << "\n";
        });

    std::ostringstream out;
    out << "Display config:\n8>< ---------------------------------------------------";
    out << "\nlayouts:";
    out << "\n# keys here are layout labels (used for atomically switching between them)";
    out << "\n# when enabling displays, surfaces should be matched in reverse recency order";
    out << "\n";
    out << "\n  my-layout:                       # the first layout is the default";
    out << "\n";
    out << "\n    displays:";
    out << "\n    # a list of displays matched by card-id and output label";

    for (auto& co : card_map)
    {
        out << "\n";
        out << "\n    - card-id: " << co.first.as_value();
        out << co.second.out.str();
    }

    out << "8>< ---------------------------------------------------";
    mir::log_info(out.str());
}

