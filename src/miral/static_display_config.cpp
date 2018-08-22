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

// Scale is not supported when compositing. https://github.com/MirServer/mir/issues/552
#define MIR_SCALE_NOT_SUPPORTED

namespace mg = mir::graphics;
using namespace mir::geometry;

namespace
{
static constexpr char const* const output_id = "output_id";
static constexpr char const* const position = "position";
static constexpr char const* const mode = "mode";
#ifndef MIR_SCALE_NOT_SUPPORTED
static constexpr char const* const scale = "scale";
#endif
static constexpr char const* const orientation = "orientation";


static constexpr char const* const orientation_value[] =
    { "normal", "left", "inverted", "right" };

static auto as_string(MirOrientation orientation) -> char const*
{
    return orientation_value[orientation/90];
}

static auto as_orientation(std::string const& orientation) -> MirOrientation
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

    auto const config = LoadFile(filename);
    if (!config.IsDefined())
        puts("!config.IsDefined()");

    Node layouts = config["layouts"];
    if (!layouts.IsDefined())
        puts("!layouts.IsDefined()");

    switch (layouts.Type())
    {
    case NodeType::Null: puts("layouts:Null"); break;
    case NodeType::Scalar: puts("layouts:Scalar"); break;
    case NodeType::Sequence: puts("layouts:Sequence"); break;
    case NodeType::Map: puts("layouts:Map"); break;
    case NodeType::Undefined: puts("layouts:Undefined"); break;
    }

    auto const layout = begin(config["layouts"]);

    puts("*********************************************************");
    if (layout != end(config["layouts"]))
    {
        Node name_node;
        Node config_node;

        std::tie(name_node, config_node) = *layout;

        auto const layout_name = name_node.Scalar();

        std::cout << "layout_name: " << layout_name << '\n';

        Node displays = config_node["displays"];

        for (Node const card : displays)
        {
            int card_no = 0;

            if (auto const id = card["card-id"])
            {
                card_no = id.as<int>();
            }

            std::cout << "Card #" << card_no << '\n';

            for (std::pair<Node, Node> port : card)
            {
                auto const port_name = port.first.Scalar();

                if (port_name == "card-id")
                    continue;

                std::cout << "Port: " << port_name << '\n';

                auto const& port_config = port.second;

                if (port_config.IsDefined())
                {
                    // TODO remove requirement for port field
                    if (auto const p = port_config["port"])
                    {
                        puts("port");
                        int const port_no = p.as<int>();
                        std::cout << "Port #" << port_no << '\n';

                        Id const output_id{mg::DisplayConfigurationCardId{card_no}, mg::DisplayConfigurationOutputId{port_no}};
                        Config   output_config;

                        if (auto const pos = port_config[position])
                        {
                            puts(position);
                            output_config.position = Point{pos[0].as<int>(), pos[1].as<int>()};
                        }

                        if (auto const m = port_config[mode])
                        {
                            puts(mode);
                            puts(m.as<std::string>().c_str());
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
                            puts(orientation);
                            std::string const orientation = o.as<std::string>();
                            puts(orientation.c_str());
                            output_config.orientation = as_orientation(orientation);
                        }

                        this->config[output_id] = output_config;
                    }
                }
            }
        }
    }
    puts("*********************************************************");

    return;
}
catch (YAML::Exception const& x)
{
    throw mir::AbnormalExit{"ERROR: in display configuration file: '" + filename + "' : " + x.what()};
}

void miral::StaticDisplayConfig::apply_to(mg::DisplayConfiguration& conf)
{
    std::ostringstream out;
    out << "Display config:\n8>< ---------------------------------------------------";

    conf.for_each_output([&](mg::UserDisplayConfigurationOutput& conf_output)
        {
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

            auto const type = mir_output_type_name(static_cast<MirOutputType>(conf_output.type));

            out << '\n' << output_id << '=' << conf_output.card_id << '/' << conf_output.id;

            if (conf_output.connected && conf_output.modes.size() > 0)
            {
                out << ": " << position << '=' << conf_output.top_left.x << ',' << conf_output.top_left.y
                    << "; " << mode << '=' << conf_output.modes[conf_output.current_mode_index]
#ifndef MIR_SCALE_NOT_SUPPORTED
                    << "; " << scale << '=' << conf_output.scale
#endif
                    << "; " << orientation << '=' << as_string(conf_output.orientation);
            }

            out << " # " << type;

            if (!conf_output.connected || conf_output.modes.size() <= 0)
                out << " (disconnected)";

            if (conf_output.modes.size() > 0)
            {
                out << " modes: ";
                for (size_t i = 0; i < conf_output.modes.size(); ++i)
                {
                    if (i) out << ", ";
                    out << conf_output.modes[i];
                }
            }
        });

    out << "\n8>< ---------------------------------------------------";
    mir::log_info(out.str());
}

