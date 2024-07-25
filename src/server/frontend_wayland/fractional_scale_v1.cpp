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
#include "fractional-scale-v1_wrapper.h"
#include "mir/frontend/surface.h"
#include "mir/shell/surface_specification.h"
#include "wl_surface.h"
#include <wayland-server.h>
#include <wlcs/display_server.h>

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

class FractionalScaleV1: public wayland::FractionalScaleV1
{
    public:
        FractionalScaleV1(struct wl_resource* resource);
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

void mf::FractionalScaleManagerV1::get_fractional_scale(struct wl_resource* id, struct wl_resource* /*surface*/)
{
    const auto scale = 1.0;
    auto fractional_scale = new FractionalScaleV1{id};
    fractional_scale->send_preferred_scale_event(120 * scale);
}

mf::FractionalScaleV1::FractionalScaleV1(struct wl_resource* resource)
    : mir::wayland::FractionalScaleV1{resource, Version<1>{}}
{
}
