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
#include "xdg-output-unstable-v1_wrapper.h"
#include "mir/log.h"
#include "output_manager.h"
#include <boost/throw_exception.hpp>

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
    XdgOutputManagerV1(struct wl_display* display, OutputManager* const output_manager);
    ~XdgOutputManagerV1() = default;

private:
    class Instance : public wayland::XdgOutputManagerV1
    {
    public:
        Instance(wl_resource* new_resource, OutputManager* manager);

    private:
        void destroy() override;
        void get_xdg_output(wl_resource* new_output, wl_resource* output) override;

        OutputManager* const output_manager;
    };

    void bind(wl_resource* new_resource) override;

    OutputManager* const output_manager;
};

class XdgOutputV1 : public wayland::XdgOutputV1
{
public:
    XdgOutputV1(
        wl_resource* new_resource,
        graphics::DisplayConfigurationOutput const& config,
        wl_resource* wl_output_resource);

private:
    void destroy();
};

}
}

auto mf::create_xdg_output_manager_v1(struct wl_display* display, OutputManager* const output_manager)
    -> std::shared_ptr<XdgOutputManagerV1>
{
    return std::make_shared<XdgOutputManagerV1>(display, output_manager);
}

mf::XdgOutputManagerV1::XdgOutputManagerV1(struct wl_display* display, mf::OutputManager* const output_manager)
    : Global(display, Version<3>()),
      output_manager{output_manager}
{
}

void mf::XdgOutputManagerV1::bind(wl_resource* new_resource)
{
    new Instance{new_resource, output_manager};
}

mf::XdgOutputManagerV1::Instance::Instance(wl_resource* new_resource, OutputManager* manager)
    : XdgOutputManagerV1{new_resource, Version<3>()},
      output_manager{manager}
{
}

void mf::XdgOutputManagerV1::Instance::destroy()
{
    destroy_wayland_object();
}

void mf::XdgOutputManagerV1::Instance::get_xdg_output(wl_resource* new_output, wl_resource* output)
{
    bool found = false;
    auto const output_id_opt = output_manager->output_id_for(client, output);
    if (!output_id_opt)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "No output for wl_output@" + std::to_string(wl_resource_get_id(output))));
    }
    auto const output_id = output_id_opt.value();

    output_manager->display_config()->for_each_output(
        [&found, output, output_id, new_output](mg::DisplayConfigurationOutput const& config)
        {
            if (config.id == output_id)
            {
                if (found)
                {
                    BOOST_THROW_EXCEPTION(std::runtime_error(
                        "Found multiple output configs with id " + std::to_string(output_id.as_value())));
                }
                new XdgOutputV1{new_output, config, output};
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
    wl_resource* new_resource,
    mg::DisplayConfigurationOutput const& config,
    wl_resource* wl_output_resource)
    : mw::XdgOutputV1(new_resource, Version<3>())
{
    auto extents = config.extents();
    send_logical_position_event(extents.left().as_int(), extents.top().as_int());
    send_logical_size_event(extents.size.width.as_int(), extents.size.height.as_int());
    if (version_supports_name())
    {
        // TODO: Better output names that are consistant between sessions
        auto output_name = "OUT-" + std::to_string(config.id.as_value());
        send_name_event(output_name);
    }
    // not sending description is allowed
    // if (version_supports_description())
    // {
    //     send_description_event("TODO: set this");
    // }

    /* xdg-output-unstable-v1.xml:
     * For objects version 3 onwards, after all xdg_output properties have been
     * sent (when the object is created and when properties are updated), a
     * wl_output.done event is sent. This allows changes to the output
     * properties to be seen as atomic, even if they happen via multiple events.
     */
    if (wl_resource_get_version(resource) >= 3)
    {
        // TODO: Use wrapper methods once wl_output is is using a wrapper
        if (wl_resource_get_version(wl_output_resource) >= WL_OUTPUT_DONE_SINCE_VERSION)
        {
            wl_output_send_done(wl_output_resource);
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
    else
    {
        /* xdg-output-unstable-v1.xml:
         * For objects version 3 onwards, this event is deprecated. Compositors
         * are not required to send it anymore and must send wl_output.done
         * instead.
         */
        send_done_event();
    }
}

void mf::XdgOutputV1::destroy()
{
    destroy_wayland_object();
}
