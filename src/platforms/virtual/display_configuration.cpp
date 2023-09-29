/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "display_configuration.h"
#include "platform.h"

namespace mg = mir::graphics;
namespace mgv = mir::graphics::virt;
namespace geom = mir::geometry;

int mgv::DisplayConfiguration::last_output_id{0};

mg::DisplayConfigurationOutput mgv::DisplayConfiguration::build_output(mgv::VirtualOutputConfig const& config)
{
    if (config.sizes.size() == 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("An output must be specified with at least one size"));
    std::vector<DisplayConfigurationMode> configuration_modes;
    for (auto size : config.sizes)
        configuration_modes.push_back({size, 60.0});

    last_output_id++;
    return  DisplayConfigurationOutput{
        mg::DisplayConfigurationOutputId{last_output_id},
        mg::DisplayConfigurationCardId{0},
        mg::DisplayConfigurationLogicalGroupId{0},
        mg::DisplayConfigurationOutputType::unknown,
        {MirPixelFormat ::mir_pixel_format_argb_8888},
        std::move(configuration_modes),
        0,
        config.sizes[0],
        true,
        true,
        {0, 0},
        0,
        MirPixelFormat::mir_pixel_format_argb_8888,
        mir_power_mode_on,
        MirOrientation::mir_orientation_normal,
        1.f,
        mir_form_factor_monitor,
        mir_subpixel_arrangement_unknown,
        {},
        mir_output_gamma_unsupported,
        {},
        {}};
}

mgv::DisplayConfiguration::DisplayConfiguration(const std::vector<DisplayConfigurationOutput> &outputs)
    : configuration{outputs}
{
}

mgv::DisplayConfiguration::DisplayConfiguration(DisplayConfiguration const& other)
    : mg::DisplayConfiguration(),
      configuration{other.configuration}
{
}

void mgv::DisplayConfiguration::for_each_output(
    std::function<void(const DisplayConfigurationOutput &)> f) const
{
    for (auto const& output: configuration)
        f(output);
}

void mgv::DisplayConfiguration::for_each_output(std::function<void(UserDisplayConfigurationOutput & )> f)
{
    for (auto& output: configuration)
    {
        mg::UserDisplayConfigurationOutput user(output);
        f(user);
    }
}

std::unique_ptr<mg::DisplayConfiguration> mgv::DisplayConfiguration::clone() const
{
    return std::make_unique<mgv::DisplayConfiguration>(*this);
}

bool mgv::DisplayConfiguration::valid() const
{
    return mg::DisplayConfiguration::valid();
}
