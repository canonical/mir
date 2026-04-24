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

#include "fractional_scale_v1.h"
#include "protocol_error.h"

#include <mir/graphics/display_configuration.h>
#include "wl_surface.h"

#include <boost/throw_exception.hpp>
#include <memory>

namespace mf = mir::frontend;


mf::FractionalScaleManagerV1::FractionalScaleManagerV1()
{
}


auto mf::FractionalScaleManagerV1::get_fractional_scale(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface)
    -> std::shared_ptr<wayland_rs::WpFractionalScaleV1Impl>
{
    auto surf = WlSurface::from(&surface.value());
    if (surf->get_fractional_scale())
    {
        BOOST_THROW_EXCEPTION(mir::wayland_rs::ProtocolError(
            object_id(), Error::fractional_scale_exists, "Surface already has a fractional scale component attached"));
    }

    auto const result = std::make_shared<FractionalScaleV1>();
    surf->set_fractional_scale(result);
    return result;
}

mf::FractionalScaleV1::FractionalScaleV1()
    : surface_outputs()
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
