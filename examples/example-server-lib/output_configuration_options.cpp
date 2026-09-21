/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
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

#include "output_configuration_options.h"

#include <mir/graphics/display_configuration.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
char const* const display_config_opt = "display-config";
char const* const clone_opt_val = "clone";
char const* const sidebyside_opt_val = "sidebyside";
char const* const single_opt_val = "single";

char const* const display_alpha_opt = "translucent";
char const* const display_alpha_descr = "Select a display mode with alpha channel. [{on,off}]";
char const* const display_alpha_off = "off";
char const* const display_alpha_on = "on";

char const* const display_scale_opt = "display-scale";
char const* const display_scale_descr = "Pixel scale for all displays, e.g. 2.0.";
auto const display_scale_default = 1.0;
auto const display_scale_min = 0.01;
auto const display_scale_max = 100.0;

char const* const display_autoscale_opt = "display-autoscale";
char const* const display_autoscale_descr =
    "Automatically set pixel scale for displays so they have specified logical height in pixels, e.g. 1080.";

auto contains_alpha(MirPixelFormat format) -> bool
{
    return format == mir_pixel_format_abgr_8888 || format == mir_pixel_format_argb_8888;
}

auto select_mode_index(uint32_t mode_index, std::vector<mg::DisplayConfigurationMode> const& modes) -> uint32_t
{
    return mode_index < modes.size() ? mode_index : 0;
}
}

class OutputConfigurationOptions::Strategy : public miral::OutputConfiguration::Strategy
{
public:
    enum class Layout { clone, sidebyside, single };

    void set_layout(std::string const& value);
    void set_with_alpha(std::string const& value);
    void set_scale(double value);
    void set_autoscale(std::optional<int> const& value);

    void apply_configuration(std::span<mg::UserDisplayConfigurationOutput> outputs) override;
    void confirm_configuration(std::span<mg::UserDisplayConfigurationOutput const> outputs) override;

private:
    void apply_scale_to(mg::UserDisplayConfigurationOutput& output) const;
    void apply_format_to(mg::UserDisplayConfigurationOutput& output) const;

    Layout layout{Layout::sidebyside};
    bool with_alpha{false};
    double scale{display_scale_default};
    std::optional<int> autoscale_target{};
};

void OutputConfigurationOptions::Strategy::set_layout(std::string const& value)
{
    if (value == clone_opt_val)
        layout = Layout::clone;
    else if (value == sidebyside_opt_val)
        layout = Layout::sidebyside;
    else if (value == single_opt_val)
        layout = Layout::single;
    else
        throw std::runtime_error{std::format("Unrecognised {} value: {}", display_config_opt, value)};
}

void OutputConfigurationOptions::Strategy::set_with_alpha(std::string const& value)
{
    if (value != display_alpha_on && value != display_alpha_off)
        throw std::runtime_error{std::format("Unrecognised {} value: {}", display_alpha_opt, value)};

    with_alpha = (value == display_alpha_on);
}

void OutputConfigurationOptions::Strategy::set_scale(double value)
{
    if (value < display_scale_min || value > display_scale_max)
    {
        throw std::runtime_error{
            std::format("Invalid scale {}, must be between {} and {}", value, display_scale_min, display_scale_max)};
    }

    scale = value;
}

void OutputConfigurationOptions::Strategy::set_autoscale(std::optional<int> const& value)
{
    if (value && scale != display_scale_default)
        throw std::runtime_error{std::format("{} can't be used with {}", display_scale_opt, display_autoscale_opt)};

    autoscale_target = value;
}

void OutputConfigurationOptions::Strategy::apply_configuration(std::span<mg::UserDisplayConfigurationOutput> outputs)
{
    geom::X next_x{0};
    auto output_in_use = false;

    for (auto& output : outputs)
    {
        if (!output.connected || output.modes.empty() || (output_in_use && layout == Layout::single))
        {
            output.used = false;
            output.power_mode = mir_power_mode_off;
            continue;
        }

        output.used = true;
        output.power_mode = mir_power_mode_on;
        output.orientation = mir_orientation_normal;
        output.current_mode_index = select_mode_index(output.preferred_mode_index, output.modes);
        apply_format_to(output);
        apply_scale_to(output);

        output.top_left = geom::Point{layout == Layout::sidebyside ? next_x : geom::X{0}, geom::Y{0}};
        next_x = output.top_left.x + as_delta(output.extents().size.width);
        output_in_use = true;
    }
}

void OutputConfigurationOptions::Strategy::confirm_configuration(std::span<mg::UserDisplayConfigurationOutput const>)
{
}

void OutputConfigurationOptions::Strategy::apply_format_to(mg::UserDisplayConfigurationOutput& output) const
{
    auto const format = std::ranges::find_if(
        output.pixel_formats, [this](MirPixelFormat format) { return contains_alpha(format) == with_alpha; });

    // keep the default setting if nothing was found
    if (format != output.pixel_formats.end())
        output.current_format = *format;
}

void OutputConfigurationOptions::Strategy::apply_scale_to(mg::UserDisplayConfigurationOutput& output) const
{
    if (!autoscale_target)
    {
        output.scale = static_cast<float>(scale);
        return;
    }

    auto const& mode_size = output.modes[output.current_mode_index].size;
    auto const output_height = (output.orientation == mir_orientation_normal ||
                                output.orientation == mir_orientation_inverted) ?
        mode_size.height.as_int() : mode_size.width.as_int();

    static auto constexpr steps = 4.0f;
    output.scale = std::round((steps * output_height) / *autoscale_target) / steps;
}

OutputConfigurationOptions::OutputConfigurationOptions() :
    strategy{std::make_shared<Strategy>()},
    output_configuration{strategy}
{
    options.push_back(miral::pre_init(miral::ConfigurationOption{
        std::function<void(std::string const&)>{[strategy=strategy](std::string const& value)
            { strategy->set_layout(value); }},
        display_config_opt,
        std::format("Display configuration:\n"
                    " - `{}`: all screens show the same content.\n"
                    " - `{}`: each screen placed to the right of the previous one.\n"
                    " - `{}`: only the first screen used.",
                    clone_opt_val, sidebyside_opt_val, single_opt_val),
        sidebyside_opt_val}));

    options.push_back(miral::pre_init(miral::ConfigurationOption{
        std::function<void(std::string const&)>{[strategy=strategy](std::string const& value)
            { strategy->set_with_alpha(value); }},
        display_alpha_opt, display_alpha_descr, display_alpha_off}));

    options.push_back(miral::pre_init(miral::ConfigurationOption{
        std::function<void(double)>{[strategy=strategy](double value) { strategy->set_scale(value); }},
        display_scale_opt, display_scale_descr, display_scale_default}));

    options.push_back(miral::pre_init(miral::ConfigurationOption{
        std::function<void(std::optional<int> const&)>{[strategy=strategy](std::optional<int> const& value)
            { strategy->set_autoscale(value); }},
        display_autoscale_opt, display_autoscale_descr}));
}

void OutputConfigurationOptions::operator()(mir::Server& server) const
{
    for (auto const& option : options)
        option(server);

    output_configuration(server);
}
