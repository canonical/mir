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

#include <miral/live_config.h>

#include <mir/graphics/display_configuration.h>
#include <mir/log.h>
#include <mir/logging/tag.h>
#include <mir/synchronised.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mlc = miral::live_config;

namespace
{
mir::logging::Tag const& display_config_tag =
    mir::logging::create_tag(mir::logging::graphics(), "display-config");

char const* const clone_val = "clone";
char const* const sidebyside_val = "sidebyside";
char const* const single_val = "single";

auto const display_scale_default = 1.0f;
auto const display_scale_min = 0.01f;
auto const display_scale_max = 100.0f;

auto contains_alpha(MirPixelFormat format) -> bool
{
    return format == mir_pixel_format_abgr_8888 || format == mir_pixel_format_argb_8888;
}

auto select_mode_index(uint32_t mode_index, std::vector<mg::DisplayConfigurationMode> const& modes) -> uint32_t
{
    return mode_index < modes.size() ? mode_index : 0;
}
}

struct OutputConfigurationOptions::Settings
{
    enum class Layout { clone, sidebyside, single };

    Layout layout{Layout::sidebyside};
    bool with_alpha{false};
    float scale{display_scale_default};
    std::optional<int> autoscale_target{};
};

/// Settings are accumulated as the individual attributes are updated and only applied
/// (as a new, immutable, Strategy) once the update transaction completes
class OutputConfigurationOptions::State
{
public:
    template<typename Update>
    requires std::invocable<Update&, Settings&>
    void update(Update&& update) { std::forward<Update>(update)(*settings.lock()); }

    auto get() const -> Settings { return *settings.lock(); }

private:
    mir::Synchronised<Settings> mutable settings;
};

class OutputConfigurationOptions::Strategy : public miral::OutputConfiguration::Strategy
{
public:
    explicit Strategy(Settings const& settings) : settings{settings} {}

    void apply_configuration(std::span<mg::UserDisplayConfigurationOutput> outputs) override;
    void confirm_configuration(std::span<mg::UserDisplayConfigurationOutput const> outputs) override;

private:
    using Layout = Settings::Layout;

    void apply_scale_to(mg::UserDisplayConfigurationOutput& output) const;
    void apply_format_to(mg::UserDisplayConfigurationOutput& output) const;

    Settings const settings;
};

void OutputConfigurationOptions::Strategy::apply_configuration(std::span<mg::UserDisplayConfigurationOutput> outputs)
{
    geom::X next_x{0};
    auto output_in_use = false;

    for (auto& output : outputs)
    {
        if (!output.connected || output.modes.empty() || (output_in_use && settings.layout == Layout::single))
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

        output.top_left = geom::Point{settings.layout == Layout::sidebyside ? next_x : geom::X{0}, geom::Y{0}};
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
        output.pixel_formats,
        [this](MirPixelFormat format) { return contains_alpha(format) == settings.with_alpha; });

    // keep the default setting if nothing was found
    if (format != output.pixel_formats.end())
        output.current_format = *format;
}

void OutputConfigurationOptions::Strategy::apply_scale_to(mg::UserDisplayConfigurationOutput& output) const
{
    if (!settings.autoscale_target)
    {
        output.scale = settings.scale;
        return;
    }

    auto const& mode_size = output.modes[output.current_mode_index].size;
    auto const output_height = (output.orientation == mir_orientation_normal ||
                                output.orientation == mir_orientation_inverted) ?
        mode_size.height.as_int() : mode_size.width.as_int();

    static auto constexpr steps = 4.0f;
    output.scale = std::round((steps * output_height) / *settings.autoscale_target) / steps;
}

OutputConfigurationOptions::OutputConfigurationOptions(mlc::Store& config_store) :
    state{std::make_shared<State>()},
    output_configuration{std::make_shared<Strategy>(Settings{})}
{
    config_store.add_string_attribute(
        {"display", "layout"},
        std::format("Display configuration:\n"
                    " - `{}`: all screens show the same content.\n"
                    " - `{}`: each screen placed to the right of the previous one.\n"
                    " - `{}`: only the first screen used.",
                    clone_val, sidebyside_val, single_val),
        sidebyside_val,
        [state=state](mlc::Key const& key, std::optional<std::string_view> value)
        {
            auto const layout = value.value_or(sidebyside_val);

            if (layout == clone_val)
                state->update([](Settings& s) { s.layout = Settings::Layout::clone; });
            else if (layout == sidebyside_val)
                state->update([](Settings& s) { s.layout = Settings::Layout::sidebyside; });
            else if (layout == single_val)
                state->update([](Settings& s) { s.layout = Settings::Layout::single; });
            else
                mir::log_warning({display_config_tag}, "Config key '{}' has invalid value: {}", key, layout);
        });

    config_store.add_bool_attribute(
        {"display", "translucent"},
        "Select a display mode with alpha channel.",
        false,
        [state=state](mlc::Key const&, std::optional<bool> value)
        {
            state->update([with_alpha=value.value_or(false)](Settings& s) { s.with_alpha = with_alpha; });
        });

    config_store.add_float_attribute(
        {"display", "scale"},
        std::format("Pixel scale for all displays, e.g. 2.0. (Between {} and {})",
                    display_scale_min, display_scale_max),
        display_scale_default,
        [state=state](mlc::Key const& key, std::optional<float> value)
        {
            auto const scale = value.value_or(display_scale_default);

            if (scale < display_scale_min || scale > display_scale_max)
            {
                mir::log_warning({display_config_tag}, "Config key '{}' has invalid value: {}", key, scale);
                return;
            }

            state->update([scale](Settings& s) { s.scale = scale; });
        });

    config_store.add_int_attribute(
        {"display", "autoscale"},
        "Automatically set pixel scale for displays so they have specified logical height in pixels, e.g. 1080.",
        [state=state](mlc::Key const& key, std::optional<int> value)
        {
            if (value && *value <= 0)
            {
                mir::log_warning({display_config_tag}, "Config key '{}' has invalid value: {}", key, *value);
                return;
            }

            state->update([value](Settings& s) { s.autoscale_target = value; });
        });

    config_store.on_done([state=state, output_configuration=output_configuration]() mutable
        {
            auto const settings = state->get();

            if (settings.autoscale_target && settings.scale != display_scale_default)
                mir::log_warning({display_config_tag}, "'display_scale' is ignored when 'display_autoscale' is set");

            output_configuration.update_strategy(std::make_shared<Strategy>(settings));
        });
}

void OutputConfigurationOptions::operator()(mir::Server& server) const
{
    output_configuration(server);
}
