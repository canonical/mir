/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/test/display_config_matchers.h"
#include "mir/graphics/display_configuration.h"
#include "mir_protobuf.pb.h"
#include "mir_toolkit/client_types.h"
#include "mir_toolkit/mir_display_configuration.h"
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mp = mir::protobuf;
namespace geom = mir::geometry;
namespace mt = mir::test;

namespace
{

size_t find_mode_index(MirOutput const* output, MirOutputMode const* mode)
{
    for (int i = 0; i < mir_output_get_num_modes(output); ++i)
    {
        if (mir_output_get_mode(output, i) == mode)
        {
            return i;
        }
    }
    return static_cast<size_t>(-1);
}

class TestDisplayConfiguration : public mg::DisplayConfiguration
{
public:
    TestDisplayConfiguration(mp::DisplayConfiguration const& protobuf_config)
    {
        /* Outputs */
        for (int i = 0; i < protobuf_config.display_output_size(); i++)
        {
            auto const& protobuf_output = protobuf_config.display_output(i);

            mir::optional_value<geom::Size> custom_logical_size;
            if (protobuf_output.has_custom_logical_size() &&
                protobuf_output.custom_logical_size() &&
                protobuf_output.has_logical_width() &&
                protobuf_output.has_logical_height())
            {
                custom_logical_size = {protobuf_output.logical_width(),
                                       protobuf_output.logical_height()};
            }

            mg::DisplayConfigurationOutput display_output
            {
                mg::DisplayConfigurationOutputId(protobuf_output.output_id()),
                mg::DisplayConfigurationCardId(0), // Not supported
                static_cast<mg::DisplayConfigurationOutputType>(protobuf_output.type()),
                {},
                {},
                protobuf_output.preferred_mode(),
                geom::Size{protobuf_output.physical_width_mm(),
                           protobuf_output.physical_height_mm()},
                static_cast<bool>(protobuf_output.connected()),
                static_cast<bool>(protobuf_output.used()),
                geom::Point{protobuf_output.position_x(),
                            protobuf_output.position_y()},
                protobuf_output.current_mode(),
                static_cast<MirPixelFormat>(protobuf_output.current_format()),
                static_cast<MirPowerMode>(protobuf_output.power_mode()),
                static_cast<MirOrientation>(protobuf_output.orientation()),
                protobuf_output.scale_factor(),
                mir_form_factor_monitor,
                mir_subpixel_arrangement_unknown,
                {},
                mir_output_gamma_unsupported,
                {},
                custom_logical_size
            };

            /* Modes */
            std::vector<mg::DisplayConfigurationMode> modes;
            for (int n = 0; n < protobuf_output.mode_size(); n++)
            {
                auto const& protobuf_mode = protobuf_output.mode(n);
                modes.push_back(
                    {
                        geom::Size{protobuf_mode.horizontal_resolution(),
                                   protobuf_mode.vertical_resolution()},
                        protobuf_mode.refresh_rate()
                    });
            }
            display_output.modes = modes;

            /* Pixel formats */
            std::vector<MirPixelFormat> pixel_formats;
            for (int n = 0; n < protobuf_output.pixel_format_size(); n++)
            {
                pixel_formats.push_back(
                    static_cast<MirPixelFormat>(protobuf_output.pixel_format(n)));
            }
            display_output.pixel_formats = pixel_formats;

            outputs.push_back(display_output);
        }
    }

    TestDisplayConfiguration(MirDisplayConfiguration const& client_config)
    {
        /* Outputs */
        for (size_t i = 0; i < client_config.num_outputs; i++)
        {
            auto const& client_output = client_config.outputs[i];
            mg::DisplayConfigurationOutput display_output
            {
                mg::DisplayConfigurationOutputId(client_output.output_id),
                mg::DisplayConfigurationCardId(0), // Not supported
                static_cast<mg::DisplayConfigurationOutputType>(client_output.type),
                {},
                {},
                client_output.preferred_mode,
                geom::Size{client_output.physical_width_mm,
                           client_output.physical_height_mm},
                static_cast<bool>(client_output.connected),
                static_cast<bool>(client_output.used),
                geom::Point{client_output.position_x,
                            client_output.position_y},
                client_output.current_mode,
                client_output.current_format,
                static_cast<MirPowerMode>(client_output.power_mode),
                static_cast<MirOrientation>(client_output.orientation),
                1.0f,
                mir_form_factor_monitor,
                mir_subpixel_arrangement_unknown,
                {},
                mir_output_gamma_unsupported,
                {},
                {}
            };

            /* Modes */
            std::vector<mg::DisplayConfigurationMode> modes;
            for (size_t n = 0; n < client_output.num_modes; n++)
            {
                auto const& client_mode = client_output.modes[n];
                modes.push_back(
                    {
                        geom::Size{client_mode.horizontal_resolution,
                                   client_mode.vertical_resolution},
                        client_mode.refresh_rate
                    });
            }
            display_output.modes = modes;

            /* Pixel formats */
            std::vector<MirPixelFormat> pixel_formats;
            for (size_t n = 0; n < client_output.num_output_formats; n++)
            {
                pixel_formats.push_back(client_output.output_formats[n]);
            }
            display_output.pixel_formats = pixel_formats;

            outputs.push_back(display_output);
        }
    }

    TestDisplayConfiguration(MirDisplayConfig const* config)
    {
        /* Outputs */
        for (int i = 0; i < mir_display_config_get_num_outputs(config); i++)
        {
            auto const client_output = mir_display_config_get_output(config, i);

            mir::optional_value<geom::Size> custom_logical_size;
            if (mir_output_has_custom_logical_size(client_output))
            {
                custom_logical_size =
                {
                    mir_output_get_logical_width(client_output),
                    mir_output_get_logical_height(client_output)
                };
            }

            mg::DisplayConfigurationOutput display_output
                {
                    mg::DisplayConfigurationOutputId(mir_output_get_id(client_output)),
                    mg::DisplayConfigurationCardId(0), // Not supported
                    static_cast<mg::DisplayConfigurationOutputType>(mir_output_get_type(client_output)),
                    {},
                    {},
                    static_cast<uint32_t>(find_mode_index(client_output, mir_output_get_preferred_mode(client_output))),
                    geom::Size{mir_output_get_physical_width_mm(client_output),
                               mir_output_get_physical_height_mm(client_output)},
                    mir_output_get_connection_state(client_output) == mir_output_connection_state_connected,
                    mir_output_is_enabled(client_output),
                    geom::Point{mir_output_get_position_x(client_output),
                        mir_output_get_position_y(client_output)},
                    static_cast<uint32_t>(find_mode_index(client_output, (mir_output_get_current_mode(client_output)))),
                    mir_output_get_current_pixel_format(client_output),
                    mir_output_get_power_mode(client_output),
                    mir_output_get_orientation(client_output),
                    mir_output_get_scale_factor(client_output),
                    mir_form_factor_monitor,
                    mir_subpixel_arrangement_unknown,
                    {},
                    mir_output_gamma_unsupported,
                    {},
                    custom_logical_size
                };

            /* Modes */
            std::vector<mg::DisplayConfigurationMode> modes;
            for (int n = 0; n < mir_output_get_num_modes(client_output); n++)
            {
                auto const client_mode = mir_output_get_mode(client_output, n);
                modes.push_back(
                    {
                        geom::Size{
                            mir_output_mode_get_width(client_mode),
                            mir_output_mode_get_height(client_mode)},
                        mir_output_mode_get_refresh_rate(client_mode)
                    });
            }
            display_output.modes = modes;

            /* Pixel formats */
            std::vector<MirPixelFormat> pixel_formats;
            for (int n = 0; n < mir_output_get_num_pixel_formats(client_output); n++)
            {
                pixel_formats.push_back(mir_output_get_pixel_format(client_output, n));
            }
            display_output.pixel_formats = pixel_formats;

            outputs.push_back(display_output);
        }
    }

    TestDisplayConfiguration(TestDisplayConfiguration const& other)
        : mg::DisplayConfiguration(),
          outputs{other.outputs}
    {
    }

    void for_each_card(std::function<void(mg::DisplayConfigurationCard const&)> /*f*/) const override
    {
    }

    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const override
    {
        for (auto const& output : outputs)
            f(output);
    }

    void for_each_output(std::function<void(mg::UserDisplayConfigurationOutput&)> f) override
    {
        for (auto& output : outputs)
        {
            mg::UserDisplayConfigurationOutput user(output);
            f(user);
        }
    }

    std::unique_ptr<mg::DisplayConfiguration> clone() const override
    {
        return std::make_unique<TestDisplayConfiguration>(*this);
    }

private:
    std::vector<mg::DisplayConfigurationOutput> outputs;
};

}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    mg::DisplayConfiguration const& config1,
    mg::DisplayConfiguration const& config2)
{
    using namespace testing;
    bool failure = false;

    /* Outputs */
    std::vector<mg::DisplayConfigurationOutput> outputs1;
    std::vector<mg::DisplayConfigurationOutput> outputs2;

    config1.for_each_output(
        [&outputs1](mg::DisplayConfigurationOutput const& output)
        {
            outputs1.push_back(output);
        });
    config2.for_each_output(
        [&outputs2](mg::DisplayConfigurationOutput const& output)
        {
            outputs2.push_back(output);
        });

    failure |= !ExplainMatchResult(UnorderedElementsAreArray(outputs1), outputs2, listener);

    return !failure;
}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    MirDisplayConfiguration const& client_config,
    mg::DisplayConfiguration const& display_config)
{
    TestDisplayConfiguration config1{client_config};
    return compare_display_configurations(listener, config1, display_config);
}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    mp::DisplayConfiguration const& protobuf_config,
    mg::DisplayConfiguration const& display_config)
{
    TestDisplayConfiguration config1{protobuf_config};
    return compare_display_configurations(listener, config1, display_config);
}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    MirDisplayConfiguration const* client_config1,
    MirDisplayConfiguration const* client_config2)
{
    TestDisplayConfiguration config1{*client_config1};
    TestDisplayConfiguration config2{*client_config2};
    return compare_display_configurations(listener, config1, config2);
}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    MirDisplayConfiguration const& client_config,
    mp::DisplayConfiguration const& protobuf_config)
{
    TestDisplayConfiguration config1{client_config};
    TestDisplayConfiguration config2{protobuf_config};
    return compare_display_configurations(listener, config1, config2);
}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    graphics::DisplayConfiguration const& display_config1,
    MirDisplayConfiguration const* display_config2)
{
    TestDisplayConfiguration config2{*display_config2};
    return compare_display_configurations(listener, display_config1, config2);
}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    std::shared_ptr<mg::DisplayConfiguration const> const& display_config1,
    mg::DisplayConfiguration const& display_config2)
{
    return compare_display_configurations(listener, *display_config1, display_config2);
}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    MirDisplayConfiguration const* display_config2,
    graphics::DisplayConfiguration const& display_config1)
{
    TestDisplayConfiguration config2{*display_config2};
    return compare_display_configurations(listener, display_config1, config2);
}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    MirDisplayConfig const* client_config,
    mg::DisplayConfiguration const& server_config)
{
    TestDisplayConfiguration translated_config{client_config};
    return compare_display_configurations(listener, server_config, translated_config);
}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    mg::DisplayConfiguration const& server_config,
    MirDisplayConfig const* client_config)
{
    TestDisplayConfiguration translated_config{client_config};
    return compare_display_configurations(listener, server_config, translated_config);
}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    MirDisplayConfig const* config1,
    MirDisplayConfig const* config2)
{
    TestDisplayConfiguration translated_config_one{config1};
    TestDisplayConfiguration translated_config_two{config2};
    return compare_display_configurations(listener, translated_config_one, translated_config_two);
}

bool mt::compare_display_configurations(
    testing::MatchResultListener* listener,
    std::shared_ptr<mg::DisplayConfiguration> const& config1,
    MirDisplayConfig const* config2)
{
    TestDisplayConfiguration translated_config_two{config2};
    return compare_display_configurations(listener, *config1, translated_config_two);
}
