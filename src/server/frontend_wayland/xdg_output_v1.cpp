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

#include "output_manager.h"

#include "wayland_rs/src/ffi.rs.h"

#include "weak.h"

#include <optional>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mw = mir::wayland;
namespace mwrs = mir::wayland;

namespace
{
class XdgOutputV1 : public mwrs::XdgOutputV1, public mf::OutputConfigListener
{
public:
    XdgOutputV1(
        std::shared_ptr<mwrs::Client> const& client,
        rust::Box<mwrs::XdgOutputV1Middleware> instance,
        uint32_t object_id,
        mf::OutputGlobal& output_global,
        mwrs::Output* wl_output);
    ~XdgOutputV1();

    auto destroyed_flag() const -> std::shared_ptr<bool const> { return mwrs::XdgOutputV1::destroyed_flag(); }

private:
    auto output_config_changed(mg::DisplayConfigurationOutput const& config) -> bool override;

    float const geometry_scale;
    std::optional<geom::Point> cached_position;
    std::optional<geom::Size> cached_size;
    mw::Weak<mf::OutputGlobal> const output_global;
};

XdgOutputV1::XdgOutputV1(
    std::shared_ptr<mwrs::Client> const& client,
    rust::Box<mwrs::XdgOutputV1Middleware> instance,
    uint32_t object_id,
    mf::OutputGlobal& output_global,
    mwrs::Output* wl_output)
    : mwrs::XdgOutputV1{client, std::move(instance), object_id},
      geometry_scale{client->output_geometry_scale()},
      output_global{mw::make_weak(&output_global)}
{
    output_global.add_listener(this);

    // Name may only be sent the first time
    send_name_event_if_supported(output_global.current_config().name);

    // not sending description is allowed

    output_config_changed(output_global.current_config());

    /* xdg-output-unstable-v1.xml:
     * For objects version 3 onwards, after all xdg_output properties have been
     * sent (when the object is created and when properties are updated), a
     * wl_output.done event is sent. This allows changes to the output
     * properties to be seen as atomic, even if they happen via multiple events.
     */
    if (get_box()->version() >= 3 && wl_output)
    {
        wl_output->send_done_event_if_supported();
    }
}

XdgOutputV1::~XdgOutputV1()
{
    if (output_global)
    {
        output_global.value().remove_listener(this);
    }
}

auto XdgOutputV1::output_config_changed(mg::DisplayConfigurationOutput const& config) -> bool
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
    if (get_box()->version() < 3)
    {
        send_done_event();
    }

    // wl_output.done is sent by the caller or once the output change we're responding to has completed

    return true;
}
}

mf::XdgOutputManagerV1::XdgOutputManagerV1(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::XdgOutputManagerV1Middleware> instance,
    uint32_t object_id)
    : wayland::XdgOutputManagerV1{std::move(client), std::move(instance), object_id}
{
}

auto mf::XdgOutputManagerV1::get_xdg_output(
    mwrs::Weak<mwrs::Output> const& output,
    rust::Box<mwrs::XdgOutputV1Middleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::XdgOutputV1>
{
    auto* const wl_output = mwrs::Output::from<mwrs::Output>(output);
    auto& output_global = OutputGlobal::from_or_throw(wl_output);
    return std::make_shared<XdgOutputV1>(client, std::move(child_instance), child_object_id, output_global, wl_output);
}
