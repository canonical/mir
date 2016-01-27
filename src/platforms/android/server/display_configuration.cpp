/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mga = mg::android;
namespace geom = mir::geometry;

namespace
{
enum DisplayIds
{
    primary_id,
    external_id,
    virtual_id,
    max_displays
};

mg::DisplayConfigurationOutput make_virtual_config()
{
    auto const name = mga::DisplayName::virt;
    double const vrefresh_hz{60.0};
    geom::Size const mm_size{25, 25};
    auto const display_format = mir_pixel_format_argb_8888;
    geom::Point const origin{0,0};
    auto const external_mode = mir_power_mode_off;
    size_t const preferred_format_index{0};
    size_t const preferred_mode_index{0};
    bool const connected{false};
    auto const type = mg::DisplayConfigurationOutputType::lvds;
    auto const form_factor = mir_form_factor_monitor;
    float const scale{1.0f};
    std::vector<mg::DisplayConfigurationMode> external_modes;
    external_modes.emplace_back(mg::DisplayConfigurationMode{{1920,1080}, vrefresh_hz});

    return {
        as_output_id(name),
        mg::DisplayConfigurationCardId{0},
        type,
        {display_format},
        external_modes,
        preferred_mode_index,
        mm_size,
        connected,
        connected,
        origin,
        preferred_format_index,
        display_format,
        external_mode,
        mir_orientation_normal,
        scale,
        form_factor
    };
}
}

mga::DisplayConfiguration::DisplayConfiguration(
    mg::DisplayConfigurationOutput primary_config,
    MirPowerMode primary_mode,
    mg::DisplayConfigurationOutput external_config,
    MirPowerMode external_mode) :
    DisplayConfiguration(primary_config, primary_mode,
                         external_config, external_mode,
                         make_virtual_config())
{
}

mga::DisplayConfiguration::DisplayConfiguration(
    mg::DisplayConfigurationOutput primary_config,
    MirPowerMode primary_mode,
    mg::DisplayConfigurationOutput external_config,
    MirPowerMode external_mode,
    mg::DisplayConfigurationOutput virt_config) :
    configurations{
        {std::move(primary_config),
        std::move(external_config),
        std::move(virt_config)}
    },
    card{mg::DisplayConfigurationCardId{0}, max_displays}
{
    primary().power_mode = primary_mode;
    external().power_mode = external_mode;
}

mga::DisplayConfiguration::DisplayConfiguration(DisplayConfiguration const& other) :
    mg::DisplayConfiguration(),
    configurations(other.configurations),
    card(other.card)
{
}

mga::DisplayConfiguration& mga::DisplayConfiguration::operator=(DisplayConfiguration const& other)
{
    if (&other != this)
    {
        configurations = other.configurations;
        card = other.card;
    }
    return *this;
}

void mga::DisplayConfiguration::for_each_card(std::function<void(mg::DisplayConfigurationCard const&)> f) const
{
    f(card);
}

void mga::DisplayConfiguration::for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const
{
    for (auto const& configuration : configurations)
        f(configuration);
}

void mga::DisplayConfiguration::for_each_output(std::function<void(mg::UserDisplayConfigurationOutput&)> f)
{
    for (auto& configuration : configurations)
    {
        mg::UserDisplayConfigurationOutput user(configuration);
        f(user);
    }
}

std::unique_ptr<mg::DisplayConfiguration> mga::DisplayConfiguration::clone() const
{
    return std::make_unique<mga::DisplayConfiguration>(*this);
}

mg::DisplayConfigurationOutput& mga::DisplayConfiguration::primary()
{
    return configurations[primary_id];
}

mg::DisplayConfigurationOutput& mga::DisplayConfiguration::external()
{
    return configurations[external_id];
}

mg::DisplayConfigurationOutput& mga::DisplayConfiguration::virt()
{
    return configurations[virtual_id];
}

mg::DisplayConfigurationOutput& mga::DisplayConfiguration::operator[](mg::DisplayConfigurationOutputId const& disp_id)
{
    auto id = disp_id.as_value() - 1;
    if (id != primary_id && id != external_id && id != virtual_id)
        BOOST_THROW_EXCEPTION(std::invalid_argument("invalid display id"));
    return configurations[id];
}

void mga::DisplayConfiguration::set_virtual_output_to(int width, int height)
{
    auto& virt_config = virt();
    virt_config.connected = true;
    virt_config.used = true;
    virt_config.power_mode = mir_power_mode_on;
    virt_config.modes[0].size = {width, height};
}

void mga::DisplayConfiguration::disable_virtual_output()
{
    auto& virt_config = virt();
    virt_config.connected = false;
    virt_config.used = false;
    virt_config.power_mode = mir_power_mode_off;
}
