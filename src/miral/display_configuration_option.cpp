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
#include "static_display_config.h"

#include <mir/graphics/default_display_configuration_policy.h>
#include <mir/graphics/display_configuration.h>
#include <mir/options/option.h>
#include <mir/server.h>

#include <algorithm>

namespace mg = mir::graphics;

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
