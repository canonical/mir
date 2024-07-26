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

#include "mir/wayland/protocol_error.h"
#include "wl_surface.h"

#include <boost/throw_exception.hpp>
#include <memory>

namespace mir
{
namespace frontend
{

class FractionalScaleManagerV1: public wayland::FractionalScaleManagerV1
{
    public:
        FractionalScaleManagerV1(struct wl_resource* resource);

        class Global: public wayland::FractionalScaleManagerV1::Global
        {
            public:
                Global(wl_display* display);

            private:
                virtual void bind(wl_resource* new_wp_fractional_scale_manager_v1) override;
        };

    private:
        virtual void get_fractional_scale(struct wl_resource* id, struct wl_resource* surface) override;
};

}
}

namespace mf = mir::frontend;

auto mf::create_fractional_scale_v1(wl_display* display)
    -> std::shared_ptr<wayland::FractionalScaleManagerV1::Global>
{
    return std::make_shared<FractionalScaleManagerV1::Global>(display);
}

mf::FractionalScaleManagerV1::FractionalScaleManagerV1(struct wl_resource* resource)
    : mir::wayland::FractionalScaleManagerV1{resource, Version<1>{}}
{
}

mf::FractionalScaleManagerV1::Global::Global(wl_display* display)
    : wayland::FractionalScaleManagerV1::Global{display, Version<1>{}}
{
}

void mf::FractionalScaleManagerV1::Global::bind(wl_resource* new_wp_fractional_scale_manager_v1)
{
    new FractionalScaleManagerV1{new_wp_fractional_scale_manager_v1};
}

void mf::FractionalScaleManagerV1::get_fractional_scale(struct wl_resource* id, struct wl_resource* surface)
{
    auto surf = WlSurface::from(surface);
    if (surf->get_fractional_scale() != nullptr)
    {
        BOOST_THROW_EXCEPTION(mir::wayland::ProtocolError(
            resource, Error::fractional_scale_exists, "Surface already has a fractional scale component attached"));
    }

    surf->set_fractional_scale(new FractionalScaleV1{id});
}

mf::FractionalScaleV1::FractionalScaleV1(struct wl_resource* resource)
    : mir::wayland::FractionalScaleV1{resource, Version<1>{}}
{
}

void mf::FractionalScaleV1::output_entered(OutputInstance* output)
{
    surface_outputs.insert(output);
    recompute_scale();
}

void mf::FractionalScaleV1::output_left(OutputInstance* output)
{
    surface_outputs.erase(output);
    recompute_scale();
}

void mf::FractionalScaleV1::recompute_scale()
{
    // Should only take an output into account if a "significant" (25%+) area of our
    // surface resides in that output?

    auto const get_scale = [](auto const* output)
    {
        return output->global.value().current_config().scale;
    };

    auto max_element = std::max_element(
        surface_outputs.cbegin(),
        surface_outputs.cend(),
        [get_scale](OutputInstance const* output1, OutputInstance const* output2)
        {
            return get_scale(output1) < get_scale(output2);
        });

    if (max_element != surface_outputs.end())
    {
        auto const preferred_scale = get_scale(*max_element);
        send_preferred_scale_event(120 * preferred_scale);
    }
}
