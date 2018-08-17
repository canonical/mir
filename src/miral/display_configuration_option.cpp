/*
 * Copyright © 2014-2018 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miral/display_configuration_option.h"

#include <mir/abnormal_exit.h>
#include <mir/graphics/default_display_configuration_policy.h>
#include <mir/graphics/display_configuration.h>
#include <mir/log.h>
#include <mir/options/option.h>
#include <mir/server.h>
#include <mir_toolkit/mir_client_library.h>

#include <map>
#include <fstream>
#include <sstream>

namespace mg = mir::graphics;
using namespace mir::geometry;

namespace
{
char const* const display_config_opt = "display-config";
char const* const display_config_descr = "Display configuration [{clone,sidebyside,single,static=<filename>}]";

//char const* const clone_opt_val = "clone";
char const* const sidebyside_opt_val = "sidebyside";
char const* const single_opt_val = "single";
char const* const static_opt_val = "static=";

char const* const display_alpha_opt = "translucent";
char const* const display_alpha_descr = "Select a display mode with alpha[{on,off}]";

char const* const display_alpha_off = "off";
char const* const display_alpha_on = "on";

class PixelFormatSelector : public mg::DisplayConfigurationPolicy
{
public:
    PixelFormatSelector(std::shared_ptr<mg::DisplayConfigurationPolicy> const& base_policy, bool with_alpha);
    virtual void apply_to(mg::DisplayConfiguration& conf);
private:
    std::shared_ptr<mg::DisplayConfigurationPolicy> const base_policy;
    bool const with_alpha;
};

bool contains_alpha(MirPixelFormat format)
{
    return (format == mir_pixel_format_abgr_8888 ||
            format == mir_pixel_format_argb_8888);
}
}

PixelFormatSelector::PixelFormatSelector(
    std::shared_ptr<mg::DisplayConfigurationPolicy> const& base_policy,
    bool with_alpha) :
    base_policy{base_policy},
    with_alpha{with_alpha}
{}

void PixelFormatSelector::apply_to(mg::DisplayConfiguration& conf)
{
    base_policy->apply_to(conf);

    conf.for_each_output(
        [&](mg::UserDisplayConfigurationOutput& conf_output)
        {
            if (!conf_output.connected || !conf_output.used) return;

            auto const& pos = find_if(conf_output.pixel_formats.begin(),
                                      conf_output.pixel_formats.end(),
                                      [&](MirPixelFormat format) -> bool
                                          {
                                              return contains_alpha(format) == with_alpha;
                                          }
                                     );

            // keep the default settings if nothing was found
            if (pos == conf_output.pixel_formats.end())
                return;

            conf_output.current_format = *pos;
        });
}

namespace
{
class StaticDisplayConfig : public mg::DisplayConfigurationPolicy
{
public:
    StaticDisplayConfig(std::string const& filename);
    virtual void apply_to(mg::DisplayConfiguration& conf);
private:

    using Id = std::tuple<mg::DisplayConfigurationCardId, mg::DisplayConfigurationOutputId>;
    struct Config { mir::optional_value<Point> position; mir::optional_value<Size> size; };

    static constexpr char const* const output_id = "output_id";
    static constexpr char const* const position = "position";
    static constexpr char const* const size = "size";

    std::map<Id, Config> config;
};

size_t select_mode_index(size_t mode_index, std::vector<mg::DisplayConfigurationMode> const & modes)
{
    if (modes.empty())
        return std::numeric_limits<size_t>::max();

    if (mode_index >= modes.size())
        return 0;

    return mode_index;
}
}

StaticDisplayConfig::StaticDisplayConfig(std::string const& filename)
{
    std::ifstream config_file{filename};

    if (!config_file)
    {
        mir::log_warning("Cannot read static display configuration file: '" + filename + "'");
        return;
    }

    int line_count = 0;
    std::string line;
    std::istringstream in;

    while (std::getline(config_file, line))
    {
        ++line_count;

        // Strip trailing comment
        line.erase(find(begin(line), end(line), '#'), end(line));

        // Ignore blank lines
        if (line.empty())
            continue;

        in = std::istringstream{line};
        in.setf(std::ios::skipws);

        std::string property;

        in >> std::ws;
        std::getline(in, property, '=');

        if (property != output_id)
            goto error;

        int card_no = -1;
        int port_no = -1;
        char delimiter = '\0';

        if (!(in >> card_no))
            goto error;

        if (!(in >> delimiter) || delimiter != '/')
            goto error;

        if (!(in >> port_no))
            goto error;

        Id const output_id{mg::DisplayConfigurationCardId{card_no}, mg::DisplayConfigurationOutputId{port_no}};
        Config   output_config;

        if (in >> delimiter && delimiter != ':')
            goto error;

        while (in >> std::ws, std::getline(in, property, '='))
        {
            if (property == position)
            {
                int x;
                int y;

                if (!(in >> x))
                    goto error;

                if (!(in >> delimiter) || delimiter != ',')
                    goto error;

                if (!(in >> y))
                    goto error;

                output_config.position = Point{x, y};
            }
            else if (property == size)
            {
                int width;
                int height;

                if (!(in >> width))
                    goto error;

                if (!(in >> delimiter) || delimiter != 'x')
                    goto error;

                if (!(in >> height))
                    goto error;

                output_config.size = Size{width, height};
            }
            else goto error;

            if (in >> std::ws, in >> delimiter && delimiter != ';')
                goto error;
        }

        config[output_id] = output_config;
    }

    return;

error:
    std::string error;
    getline(in, error);
    throw mir::AbnormalExit{"ERROR: Syntax error in display configuration file: '" + filename +
                            "' line: " + std::to_string(line_count) +
                            " before: " + error + "'"};
}

void StaticDisplayConfig::apply_to(mg::DisplayConfiguration& conf)
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
                            if (conf_output.modes[conf_output.current_mode_index].size != conf.size.value()
                             || conf_output.modes[conf_output.current_mode_index].vrefresh_hz < mode->vrefresh_hz)
                            {
                                conf_output.current_mode_index = distance(begin(conf_output.modes), mode);
                            }
                        }
                    }
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
                 auto const& tl = conf_output.top_left;
                 auto const& sz = conf_output.modes[conf_output.current_mode_index].size;
                 out << ": " << position << '=' << tl.x << ',' << tl.y
                     << "; " << size << '=' << sz.width << 'x' << sz.height;
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

void miral::display_configuration_options(mir::Server& server)
{
    // Add choice of monitor configuration
    server.add_configuration_option(display_config_opt, display_config_descr,   sidebyside_opt_val);
    server.add_configuration_option(display_alpha_opt,  display_alpha_descr,    display_alpha_off);

    server.wrap_display_configuration_policy(
        [&](std::shared_ptr<mg::DisplayConfigurationPolicy> const& wrapped)
        -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            auto const options = server.get_options();
            auto display_layout = options->get<std::string>(display_config_opt);
            auto with_alpha = options->get<std::string>(display_alpha_opt) == display_alpha_on;

            auto layout_selector = wrapped;

            if (display_layout == sidebyside_opt_val)
                layout_selector = std::make_shared<mg::SideBySideDisplayConfigurationPolicy>();
            else if (display_layout == single_opt_val)
                layout_selector = std::make_shared<mg::SingleDisplayConfigurationPolicy>();
            else if (display_layout.compare(0, strlen(static_opt_val), static_opt_val) == 0)
                layout_selector = std::make_shared<StaticDisplayConfig>(display_layout.substr(strlen(static_opt_val)));

            // Whatever the layout select a pixel format with requested alpha
            return std::make_shared<PixelFormatSelector>(layout_selector, with_alpha);
        });
}
