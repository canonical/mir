/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "real_kms_display_configuration.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/log.h"
#include "kms_output_container.h"
#include "kms_output.h"

#include <cmath>
#include <limits>

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <algorithm>
#include <system_error>
#include <cstring>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mgk = mir::graphics::kms;
namespace geom = mir::geometry;

mgm::RealKMSDisplayConfiguration::RealKMSDisplayConfiguration(
    std::shared_ptr<KMSOutputContainer> const& displays)
    : displays{displays},
      card{mg::DisplayConfigurationCardId{0}, 0}
{
    update();
}

mgm::RealKMSDisplayConfiguration::RealKMSDisplayConfiguration(
    RealKMSDisplayConfiguration const& conf)
    : KMSDisplayConfiguration(),
      displays{conf.displays},
    // Vivid GCC is apparently confused by trying to copy-construct card from conf.card?
      card{conf.card.id, conf.card.max_simultaneous_outputs},
      outputs{conf.outputs}
{
}

mgm::RealKMSDisplayConfiguration& mgm::RealKMSDisplayConfiguration::operator=(
    RealKMSDisplayConfiguration const& conf)
{
    if (&conf != this)
    {
        displays = conf.displays;
        card = conf.card;
        outputs = conf.outputs;
    }

    return *this;
}

void mgm::RealKMSDisplayConfiguration::for_each_card(
    std::function<void(DisplayConfigurationCard const&)> f) const
{
    f(card);
}

void mgm::RealKMSDisplayConfiguration::for_each_output(
    std::function<void(DisplayConfigurationOutput const&)> f) const
{
    for (auto const& output_pair : outputs)
        f(output_pair.first);
}

void mgm::RealKMSDisplayConfiguration::for_each_output(
    std::function<void(UserDisplayConfigurationOutput&)> f)
{
    for (auto& output_pair : outputs)
    {
        UserDisplayConfigurationOutput user(output_pair.first);
        f(user);
    }
}

std::unique_ptr<mg::DisplayConfiguration> mgm::RealKMSDisplayConfiguration::clone() const
{
    return std::make_unique<RealKMSDisplayConfiguration>(*this);
}

mgm::RealKMSDisplayConfiguration::Output const&
mgm::RealKMSDisplayConfiguration::output(DisplayConfigurationOutputId id) const
{
    return outputs.at(id.as_value() - 1);
}

std::shared_ptr<mgm::KMSOutput> mgm::RealKMSDisplayConfiguration::get_output_for(
    DisplayConfigurationOutputId id) const
{
    return output(id).second;
}

size_t mgm::RealKMSDisplayConfiguration::get_kms_mode_index(
    DisplayConfigurationOutputId id,
    size_t conf_mode_index) const
{
    if (static_cast<size_t>(id.as_value()) > outputs.size())
    {
        BOOST_THROW_EXCEPTION(std::invalid_argument("Request for KMS mode index of invalid output ID"));
    }
    if (conf_mode_index > output(id).first.modes.size())
    {
        BOOST_THROW_EXCEPTION(std::invalid_argument("Request for out-of-bounds KMS mode index"));
    }

    return conf_mode_index;
}

namespace
{
void populate_default_mir_config(mg::DisplayConfigurationOutput& to_populate)
{
    to_populate.card_id = mg::DisplayConfigurationCardId{0};
    to_populate.gamma_supported = mir_output_gamma_supported;
    to_populate.orientation = mir_orientation_normal;
    to_populate.form_factor = mir_form_factor_monitor;
    to_populate.scale = 1.0f;
    to_populate.top_left = geom::Point{};
    to_populate.used = false;
    to_populate.pixel_formats = {mir_pixel_format_xrgb_8888, mir_pixel_format_argb_8888};
    to_populate.current_format = mir_pixel_format_xrgb_8888;
    to_populate.current_mode_index = std::numeric_limits<uint32_t>::max();
}
}

void mgm::RealKMSDisplayConfiguration::update()
{
    decltype(outputs) new_outputs;

    displays->update_from_hardware_state();
    displays->for_each_output(
        [this, &new_outputs](auto const& output) mutable
        {
            DisplayConfigurationOutput mir_config;

            auto const existing_output = std::find_if(
                outputs.begin(),
                outputs.end(),
                [&output](auto const& candidate)
                {
                    // Pointer comparison; is this KMSOutput object already present?
                    return candidate.second == output;
                });
            if (existing_output == outputs.end())
            {
                populate_default_mir_config(mir_config);
            }
            else
            {
                mir_config = existing_output->first;
            }

            output->update_from_hardware_state(mir_config);
            mir_config.id = DisplayConfigurationOutputId{int(new_outputs.size() + 1)};

            new_outputs.emplace_back(mir_config, output);
        });

    outputs = new_outputs;

    /*
     * This is not the true max simultaneous outputs, but it's unclear whether it's possible
     * to provide a max_simultaneous_outputs value that is useful to clients.
     */
    card.max_simultaneous_outputs = outputs.size();
}

// Compatibility means conf1 can be attained from conf2 (and vice versa)
// without recreating the display buffers (e.g. conf1 and conf2 are identical
// except one of the outputs of conf1 is rotated w.r.t. that of conf2). If
// the two outputs differ in their power state, the display buffers would need
// to be allocated/destroyed, and hence should not be considered compatible.
bool mgm::compatible(mgm::RealKMSDisplayConfiguration const& conf1, mgm::RealKMSDisplayConfiguration const& conf2)
{
    bool compatible{
        (conf1.card           == conf2.card)   &&
        (conf1.outputs.size() == conf2.outputs.size())};

    if (compatible)
    {
        unsigned int const count = conf1.outputs.size();

        for (unsigned int i = 0; i < count; ++i)
        {
            compatible &= (conf1.outputs[i].first.power_mode == conf2.outputs[i].first.power_mode);
            if (compatible)
            {
                auto clone = conf2.outputs[i].first;

                // ignore difference in orientation, scale factor, form factor, subpixel arrangement
                clone.orientation = conf1.outputs[i].first.orientation;
                clone.subpixel_arrangement = conf1.outputs[i].first.subpixel_arrangement;
                clone.scale = conf1.outputs[i].first.scale;
                clone.form_factor = conf1.outputs[i].first.form_factor;
                clone.custom_logical_size = conf1.outputs[i].first.custom_logical_size;
                compatible &= (conf1.outputs[i].first == clone);
            }
            else
                break;
        }
    }

    return compatible;
}
