/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_display_configuration_policy.h"
#include "mir/graphics/default_display_configuration_policy.h"

#include "mir/graphics/display_configuration.h"
#include "mir/server.h"
#include "mir/options/option.h"

#include <algorithm>
#include <unordered_map>
#include <stdexcept>

namespace geom = mir::geometry;
namespace me = mir::examples;
namespace mg = mir::graphics;

///\example server_example_display_configuration_policy.cpp
/// Demonstrate custom display configuration policies for "sidebyside" and "single"

char const* const me::display_config_opt = "display-config";
char const* const me::display_config_descr = "Display configuration [{clone,sidebyside,single}]";

char const* const me::clone_opt_val = "clone";
char const* const me::sidebyside_opt_val = "sidebyside";
char const* const me::single_opt_val = "single";

char const* const me::display_alpha_opt = "translucent";
char const* const me::display_alpha_descr = "Select a display mode with alpha[{on,off}]";

char const* const me::display_alpha_off = "off";
char const* const me::display_alpha_on = "on";

namespace
{
bool contains_alpha(MirPixelFormat format)
{
    return (format == mir_pixel_format_abgr_8888 ||
            format == mir_pixel_format_argb_8888);
}
}

me::PixelFormatSelector::PixelFormatSelector(std::shared_ptr<DisplayConfigurationPolicy> const& base_policy,
                                         bool with_alpha)
    : base_policy{base_policy},
    with_alpha{with_alpha}
{}

void me::PixelFormatSelector::apply_to(graphics::DisplayConfiguration & conf)
{
    base_policy->apply_to(conf);
    conf.for_each_output(
        [&](graphics::UserDisplayConfigurationOutput& conf_output)
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

void me::add_display_configuration_options_to(mir::Server& server)
{
    // Add choice of monitor configuration
    server.add_configuration_option(
        me::display_config_opt, me::display_config_descr,   me::clone_opt_val);
    server.add_configuration_option(
        me::display_alpha_opt,  me::display_alpha_descr,    me::display_alpha_off);

    server.wrap_display_configuration_policy(
        [&](std::shared_ptr<mg::DisplayConfigurationPolicy> const& wrapped)
        -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            auto const options = server.get_options();
            auto display_layout = options->get<std::string>(me::display_config_opt);
            auto with_alpha = options->get<std::string>(me::display_alpha_opt) == me::display_alpha_on;

            auto layout_selector = wrapped;

            if (display_layout == me::sidebyside_opt_val)
                layout_selector = std::make_shared<mg::SideBySideDisplayConfigurationPolicy>();
            else if (display_layout == me::single_opt_val)
                layout_selector = std::make_shared<mg::SingleDisplayConfigurationPolicy>();

            // Whatever the layout select a pixel format with requested alpha
            return std::make_shared<me::PixelFormatSelector>(layout_selector, with_alpha);
        });
}
