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

#include "miral/display_configuration_option.h"
#include "static_display_config.h"

#include <mir/graphics/default_display_configuration_policy.h>
#include <mir/graphics/display_configuration.h>
#include <mir/options/option.h>
#include <mir/server.h>

#include <algorithm>

namespace mg = mir::graphics;
using namespace mir::geometry;

namespace
{
char const* const display_config_opt = "display-config";
char const* const display_config_descr = "Display configuration:\n"
                                         " - clone: all screens show the same content.\n"
                                         " - sidebyside: each screen placed to the right of the previous one.\n"
                                         " - single: only the first screen used.\n"
                                         " - static=filename: use layout specified in <filename>.";

//char const* const clone_opt_val = "clone";
char const* const sidebyside_opt_val = "sidebyside";
char const* const single_opt_val = "single";
char const* const static_opt_val = "static=";

char const* const display_alpha_opt = "translucent";
char const* const display_alpha_descr = "Select a display mode with alpha channel. [{on,off}]";

char const* const display_alpha_off = "off";
char const* const display_alpha_on = "on";

char const* const display_scale_opt = "display-scale";
char const* const display_scale_descr = "Pixel scale for all displays, e.g. 2.0.";
char const* const display_scale_default = "1.0";

char const* const display_autoscale_opt = "display-autoscale";
char const* const display_autoscale_descr = "Automatically set pixel scale for displays so they have specified logical height in pixels, e.g. 1080.";

class PixelFormatSelector : public mg::DisplayConfigurationPolicy
{
public:
    PixelFormatSelector(std::shared_ptr<mg::DisplayConfigurationPolicy> const& base_policy, bool with_alpha);
    virtual void apply_to(mg::DisplayConfiguration& conf);
    virtual void confirm(mg::DisplayConfiguration const& conf);
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

void PixelFormatSelector::confirm(mg::DisplayConfiguration const& conf)
{
    base_policy->confirm(conf);
}

class ScaleSetter : public mg::DisplayConfigurationPolicy
{
public:
    ScaleSetter(std::shared_ptr<mg::DisplayConfigurationPolicy> const& base_policy, float with_scale)
        : base_policy{base_policy},
          with_scale{with_scale}
    {
    }

    void apply_to(mg::DisplayConfiguration& conf) override;
    void confirm(mg::DisplayConfiguration const& conf) override;
private:
    std::shared_ptr<mg::DisplayConfigurationPolicy> const base_policy;
    float const with_scale;
};

void ScaleSetter::apply_to(mg::DisplayConfiguration& conf)
{
    base_policy->apply_to(conf);
    conf.for_each_output(
        [&](mg::UserDisplayConfigurationOutput& output)
        {
            output.scale = with_scale;
        });
}

void ScaleSetter::confirm(mg::DisplayConfiguration const& conf)
{
    base_policy->confirm(conf);
}

class AutoscaleSetter : public mg::DisplayConfigurationPolicy
{
public:
    AutoscaleSetter(std::shared_ptr<mg::DisplayConfigurationPolicy> const& base_policy, int target)
        : base_policy{base_policy},
          target{target}
    {
    }

    void apply_to(mg::DisplayConfiguration& conf) override;
    void confirm(mg::DisplayConfiguration const& conf) override;
private:
    void apply_to(mg::UserDisplayConfigurationOutput& output);
    std::shared_ptr<mg::DisplayConfigurationPolicy> const base_policy;
    int target;
};

void AutoscaleSetter::apply_to(mg::DisplayConfiguration& conf)
{
    base_policy->apply_to(conf);
    conf.for_each_output(
        [this](mg::UserDisplayConfigurationOutput& output) { if (output.connected) apply_to(output); });
}

void AutoscaleSetter::confirm(mg::DisplayConfiguration const& conf)
{
    base_policy->confirm(conf);
}

void AutoscaleSetter::apply_to(mg::UserDisplayConfigurationOutput& output)
{
    auto const output_height =
        (output.orientation == mir_orientation_normal || output.orientation == mir_orientation_inverted) ?
        output.modes[output.current_mode_index].size.height.as_int() :
        output.modes[output.current_mode_index].size.width.as_int();

    static auto const steps = 4.0;
    output.scale = roundf((steps * output_height) / target) / steps;
}

void miral::display_configuration_options(mir::Server& server)
{
    // Add choice of monitor configuration
    server.add_configuration_option(display_config_opt, display_config_descr,   sidebyside_opt_val);
    server.add_configuration_option(display_alpha_opt,  display_alpha_descr,    display_alpha_off);
    server.add_configuration_option(display_scale_opt,  display_scale_descr,    display_scale_default);
    server.add_configuration_option(display_autoscale_opt,  display_autoscale_descr, mir::OptionType::integer);

    server.wrap_display_configuration_policy(
        [&](std::shared_ptr<mg::DisplayConfigurationPolicy> const& wrapped)
        -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            auto const options = server.get_options();
            auto display_layout = options->get<std::string>(display_config_opt);
            auto with_alpha = options->get<std::string>(display_alpha_opt) == display_alpha_on;
            auto const scale_str = options->get<std::string>(display_scale_opt);
            auto const is_auto_scale = options->is_set(display_autoscale_opt);

            double scale{0};
            static double const scale_min = 0.01;
            static double const scale_max = 100.0;
            try
            {
                scale = std::stod(scale_str);
            }
            catch (std::invalid_argument const&)
            {
                mir::fatal_error("Failed to parse scale '%s' as double", scale_str.c_str());
            }
            if (scale < scale_min || scale > scale_max)
            {
                mir::fatal_error("Invalid scale %f, must be between %f and %f", scale, scale_min, scale_max);
            }

            auto layout_selector = wrapped;

            if (display_layout == sidebyside_opt_val)
            {
                layout_selector = std::make_shared<mg::SideBySideDisplayConfigurationPolicy>();
            }
            else if (display_layout == single_opt_val)
            {
                layout_selector = std::make_shared<mg::SingleDisplayConfigurationPolicy>();
            }
            else if (display_layout.compare(0, strlen(static_opt_val), static_opt_val) == 0)
            {
                if (scale != 1.0)
                {
                    mir::fatal_error("Display scale option can't be used with static display configuration");
                }
                auto sdc = std::make_shared<StaticDisplayConfig>(display_layout.substr(strlen(static_opt_val)));
                server.add_init_callback([sdc, &server]{ sdc->init_auto_reload(server); });
                layout_selector = std::move(sdc);
            }

            if (scale != 1.0)
            {
                layout_selector = std::make_shared<ScaleSetter>(layout_selector, scale);
            }

            if (is_auto_scale)
            {
                if (scale != 1.0)
                {
                    mir::fatal_error("Display scale option can't be used with autoscale");
                }

                if (display_layout.compare(0, strlen(static_opt_val), static_opt_val) == 0)
                {
                    mir::fatal_error("Display autoscale option can't be used with static display configuration");
                }

                auto const auto_scale_target = options->get<int>(display_autoscale_opt);

                layout_selector = std::make_shared<AutoscaleSetter>(layout_selector, auto_scale_target);
            }

            // Whatever the layout select a pixel format with requested alpha
            return std::make_shared<PixelFormatSelector>(layout_selector, with_alpha);
        });
}
