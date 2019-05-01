/*
 * Copyright © 2018-2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "xdg_output_v1.h"

#include "wl_surface.h"

#include "mir/log.h"
#include "output_manager.h"
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace mir
{
namespace frontend
{

class XdgOutputV1 : public wayland::XdgOutputV1
{
public:
    XdgOutputV1(
        wl_client* client,
        wl_resource* resource_parent,
        uint32_t id,
        graphics::DisplayConfigurationOutput const& config);

private:
    void destroy();
};

}
}

mf::XdgOutputManagerV1::XdgOutputManagerV1(struct wl_display* display, mf::OutputManager* const output_manager)
    : wayland::XdgOutputManagerV1(display, wayland::XdgOutputManagerV1::interface_version),
      output_manager{output_manager}
{
}

void mf::XdgOutputManagerV1::destroy(struct wl_client* client, struct wl_resource* resource)
{
    (void)client;
    destroy_wayland_object(resource);
}

void mf::XdgOutputManagerV1::get_xdg_output(
    struct wl_client* client,
    struct wl_resource* resource,
    uint32_t id,
    struct wl_resource* output)
{
    bool found = false;
    auto const output_id = output_manager->output_id_for(client, output);
    output_manager->display_config()->for_each_output(
        [&found, output_id, client, resource, id](mg::DisplayConfigurationOutput const& config)
        {
            if (config.id == output_id)
            {
                if (found)
                {
                    BOOST_THROW_EXCEPTION(std::runtime_error(
                        "Found multiple output configs with id " + std::to_string(output_id.as_value())));
                }
                new XdgOutputV1{client, resource, id, config};
                found = true;
            }
        });

    if (!found)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Did not find output config with id " + std::to_string(output_id.as_value())));
    }
}

mf::XdgOutputV1::XdgOutputV1(
    wl_client* client,
    wl_resource* resource_parent,
    uint32_t id,
    mg::DisplayConfigurationOutput const& config)
    : wayland::XdgOutputV1(client, resource_parent, id)
{
    auto extents = config.extents();
    send_logical_position_event(extents.left().as_int(), extents.top().as_int());
    send_logical_size_event(extents.size.width.as_int(), extents.size.height.as_int());
    // TODO: Better output names that are consistant between sessions
    auto output_name = "OUT-" + std::to_string(config.id.as_value());
    send_name_event(output_name);
    // not sending description is allowed
    // send_description_event("TODO: set this");
    send_done_event();
}

void mf::XdgOutputV1::destroy()
{
    destroy_wayland_object();
}
