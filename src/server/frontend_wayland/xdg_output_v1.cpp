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

#include "wl_surface.h"
#include "xdg-output-unstable-v1_wrapper.h"
#include "mir/log.h"
#include "output_manager.h"
#include "mir/wayland/client.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{

class XdgOutputManagerV1 : public wayland::XdgOutputManagerV1::Global
{
public:
    XdgOutputManagerV1(struct wl_display* display);
    ~XdgOutputManagerV1() = default;

private:
    class Instance : public wayland::XdgOutputManagerV1
    {
    public:
        Instance(wl_resource* new_resource);

    private:
        void get_xdg_output(wl_resource* new_output, wl_resource* output) override;
    };

    void bind(wl_resource* new_resource) override;
};

class XdgOutputV1 : public wayland::XdgOutputV1, OutputConfigListener
{
public:
    XdgOutputV1(
        wl_resource* new_resource,
        OutputGlobal& output_global,
        wl_resource* wl_output_resource);
    ~XdgOutputV1();

private:
    auto output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool override;

    float const geometry_scale;
    std::optional<geometry::Point> cached_position;
    std::optional<geometry::Size> cached_size;
    wayland::Weak<OutputGlobal> const output_global;
};

}
}

auto mf::create_xdg_output_manager_v1(struct wl_display* display)
    -> std::shared_ptr<mw::XdgOutputManagerV1::Global>
{
    return std::make_shared<XdgOutputManagerV1>(display);
}

mf::XdgOutputManagerV1::XdgOutputManagerV1(struct wl_display* display)
    : Global(display, Version<3>())
{
}

void mf::XdgOutputManagerV1::bind(wl_resource* new_resource)
{
    new Instance{new_resource};
}

mf::XdgOutputManagerV1::Instance::Instance(wl_resource* new_resource)
    : XdgOutputManagerV1{new_resource, Version<3>()}
{
}

void mf::XdgOutputManagerV1::Instance::get_xdg_output(wl_resource* new_output, wl_resource* output)
{
    new XdgOutputV1{new_output, OutputGlobal::from_or_throw(output), output};
}

mf::XdgOutputV1::XdgOutputV1(
    wl_resource* new_resource,
    OutputGlobal& output_global,
    wl_resource* wl_output_resource)
    : mw::XdgOutputV1(new_resource, Version<3>()),
      geometry_scale{client->output_geometry_scale()},
      output_global{mw::make_weak(&output_global)}
{
    output_global.add_listener(this);

    // Name may only be sent the first time
    send_name_event_if_supported(output_global.current_config().name);

    // not sending description is allowed
    // send_description_event_if_supported("TODO: set this");

    output_config_changed(output_global.current_config());

    /* xdg-output-unstable-v1.xml:
     * For objects version 3 onwards, after all xdg_output properties have been
     * sent (when the object is created and when properties are updated), a
     * wl_output.done event is sent. This allows changes to the output
     * properties to be seen as atomic, even if they happen via multiple events.
     */
    if (wl_resource_get_version(resource) >= 3)
    {
        auto const wl_output = mw::Output::from(wl_output_resource);
        if (wl_output->version_supports_done())
        {
            wl_output->send_done_event();
        }
        else
        {
            log_warning(
                "xdg_output_v1 is v%d (meaning Mir must send wl_output.done), "
                "but wl_output is only v%d (meaning it can't)",
                wl_resource_get_version(resource),
                wl_resource_get_version(wl_output_resource));
        }
    }
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
    if (wl_resource_get_version(resource) < 3)
    {
        send_done_event();
    }

    // wl_output.done is sent by the caller or once the output change we're responding to has completed

    return true;
}
