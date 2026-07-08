/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "wp_fractional_scale_v1.h"

#include "wayland.h"
#include "weak.h"
#include "client.h"
#include "protocol_error.h"
#include <mir/graphics/display_configuration.h>
#include "wl_surface.h"

#include <algorithm>
#include <memory>

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;

mf::FractionalScaleManagerV1::FractionalScaleManagerV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::FractionalScaleManagerV1Middleware> instance,
    uint32_t object_id)
    : mw::FractionalScaleManagerV1{std::move(client), std::move(instance), object_id}
{
}

auto mf::FractionalScaleManagerV1::get_fractional_scale(
    mw::Weak<mw::Surface> const& surface,
    rust::Box<mw::FractionalScaleV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::FractionalScaleV1>
{
    auto surf = mw::Surface::from<WlSurface>(surface);
    if (surf && surf->get_fractional_scale())
    {
        throw mw::ProtocolError{
            object_id(), Error::fractional_scale_exists, "Surface already has a fractional scale component attached"};
    }

    auto fractional_scale = std::make_shared<FractionalScaleV1>(client, std::move(child_instance), child_object_id);
    if (surf)
        surf->set_fractional_scale(fractional_scale.get());
    return fractional_scale;
}

mf::FractionalScaleV1::FractionalScaleV1(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::FractionalScaleV1Middleware> instance,
    uint32_t object_id)
    : mw::FractionalScaleV1{std::move(client), std::move(instance), object_id},
        surface_outputs()
{
}

void mf::FractionalScaleV1::output_entered(mir::graphics::DisplayConfigurationOutput const& config)
{
    surface_outputs.insert({config.id, config.scale});
    recompute_scale();
}

void mf::FractionalScaleV1::output_left(mir::graphics::DisplayConfigurationOutput const& config)
{
    surface_outputs.erase(config.id);
    recompute_scale();
}

void mf::FractionalScaleV1::scale_change_on_output(mir::graphics::DisplayConfigurationOutput const& config)
{
    surface_outputs[config.id] = config.scale;
    recompute_scale();
}

void mf::FractionalScaleV1::recompute_scale()
{
    auto max_element = std::max_element(
        surface_outputs.cbegin(),
        surface_outputs.cend(),
        [](std::pair<Id, float> const& output1, std::pair<Id, float> const& output2)
        {
            return output1.second < output2.second;
        });

    if (max_element != surface_outputs.end())
    {
        auto const preferred_scale = max_element->second;
        send_preferred_scale_event(120 * preferred_scale);
    }
}
