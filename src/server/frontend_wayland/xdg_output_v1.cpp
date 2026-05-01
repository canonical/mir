/*
 * Copyright © Canonical Ltd.
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
 */

#include "xdg_output_v1.h"
#include "client.h"

#include "wl_surface.h"
#include <mir/log.h>
#include "output_manager.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mw = mir::wayland_rs;

namespace mir
{
namespace frontend
{

class XdgOutputV1 : public wayland_rs::ZxdgOutputV1Impl, OutputConfigListener
{
public:
    XdgOutputV1(mw::Weak<OutputGlobal> const& output_global, std::shared_ptr<wayland_rs::Client> const& client, wayland_rs::Weak<wayland_rs::WlOutputImpl> const& raw_output);
    ~XdgOutputV1();

private:
    auto output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool override;

    float const geometry_scale;
    std::optional<geometry::Point> cached_position;
    std::optional<geometry::Size> cached_size;
    wayland_rs::Weak<OutputGlobal> const output_global;
};

}
}

mf::XdgOutputManagerV1::XdgOutputManagerV1(std::shared_ptr<wayland_rs::Client> const& client)
    : client{client}
{
}

auto mf::XdgOutputManagerV1::get_xdg_output(wayland_rs::Weak<wayland_rs::WlOutputImpl> const& output) -> std::shared_ptr<wayland_rs::ZxdgOutputV1Impl>
{
    return std::make_shared<XdgOutputV1>(mw::Weak(OutputGlobal::from_or_throw(&output.value()).shared_from_this()), client, output);
}

mf::XdgOutputV1::XdgOutputV1(mw::Weak<OutputGlobal> const& output_global, std::shared_ptr<wayland_rs::Client> const& client, wayland_rs::Weak<wayland_rs::WlOutputImpl> const& raw_output)
    : geometry_scale{client->output_geometry_scale()},
      output_global{output_global}
{
    output_global.value().add_listener(this);

    // Name may only be sent the first time
    send_name_event(output_global.value().current_config().name);

    // not sending description is allowed
    // send_description_event_if_supported("TODO: set this");

    output_config_changed(output_global.value().current_config());

    /* xdg-output-unstable-v1.xml:
     * For objects version 3 onwards, after all xdg_output properties have been
     * sent (when the object is created and when properties are updated), a
     * wl_output.done event is sent. This allows changes to the output
     * properties to be seen as atomic, even if they happen via multiple events.
     */
    mf::OutputInstance::from(&raw_output.value())->send_done_event();
}

mf::XdgOutputV1::~XdgOutputV1()
{
    if (output_global)
    {
        output_global.value().remove_listener(this);
    }
}

auto mf::XdgOutputV1::output_config_changed(mg::DisplayConfigurationOutput const& config) -> bool
{
    auto extents = config.extents();
    geom::Point position = as_point(as_displacement(extents.top_left) * geometry_scale);
    geom::Size size = extents.size * geometry_scale;
    if (position != cached_position)
    {
        send_logical_position_event(position.x.as_value(), position.y.as_value());
    }
    if (size != cached_size)
    {
        send_logical_size_event(size.width.as_value(), size.height.as_value());
    }
    cached_position = position;
    cached_size = size;

    /* xdg-output-unstable-v1.xml:
    * For objects version 3 onwards, this event is deprecated. Compositors
    * are not required to send it anymore and must send wl_output.done
    * instead.
    */
    send_done_event();

    // wl_output.done is sent by the caller or once the output change we're responding to has completed

    return true;
}
