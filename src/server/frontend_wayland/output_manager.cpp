/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "output_manager.h"

#include "mir_display.h"

#include <algorithm>

namespace mf = mir::frontend;
namespace mg = mir::graphics;

mf::Output::Output(wl_display* display, mg::DisplayConfigurationOutput const& initial_configuration) :
    output{make_output(display)},
    current_config{initial_configuration}
{
}

mf::Output::~Output()
{
    wl_global_destroy(output);
}

void mf::Output::handle_configuration_changed(mg::DisplayConfigurationOutput const& /*config*/)
{
}

bool mf::Output::matches_client_resource(wl_client* client, struct wl_resource* resource) const
{
    auto const rp = resource_map.find(client);

    if (rp == resource_map.end())
        return false;

    for (auto const& r : rp->second)
        if (r == resource)
            return true;

    return false;
}


void mf::Output::send_initial_config(wl_resource* client_resource, mg::DisplayConfigurationOutput const& config)
{
    wl_output_send_geometry(
        client_resource,
        config.top_left.x.as_int(),
        config.top_left.y.as_int(),
        config.physical_size_mm.width.as_int(),
        config.physical_size_mm.height.as_int(),
        WL_OUTPUT_SUBPIXEL_UNKNOWN,
        "Fake manufacturer",
        "Fake model",
        WL_OUTPUT_TRANSFORM_NORMAL);
    for (size_t i = 0; i < config.modes.size(); ++i)
    {
        auto const& mode = config.modes[i];
        wl_output_send_mode(
            client_resource,
            ((i == config.preferred_mode_index ? WL_OUTPUT_MODE_PREFERRED : 0) |
             (i == config.current_mode_index ? WL_OUTPUT_MODE_CURRENT : 0)),
            mode.size.width.as_int(),
            mode.size.height.as_int(),
            mode.vrefresh_hz * 1000);
    }

    if (wl_resource_get_version(client_resource) >= WL_OUTPUT_SCALE_SINCE_VERSION)
        wl_output_send_scale(client_resource, 1);

    if (wl_resource_get_version(client_resource) >= WL_OUTPUT_DONE_SINCE_VERSION)
        wl_output_send_done(client_resource);
}

wl_global* mf::Output::make_output(wl_display* display)
{
    return wl_global_create(
        display,
        &wl_output_interface,
        2,
        this, &on_bind);
}

void mf::Output::on_bind(wl_client* client, void* data, uint32_t version, uint32_t id)
{
    auto output = reinterpret_cast<Output*>(data);
    auto resource = wl_resource_create(
        client, &wl_output_interface,
        std::min(version, 2u), id);
    if (resource == NULL)
    {
        wl_client_post_no_memory(client);
        return;
    }

    output->resource_map[client].push_back(resource);
    wl_resource_set_destructor(resource, &resource_destructor);
    wl_resource_set_user_data(resource, &(output->resource_map));

    send_initial_config(resource, output->current_config);
}

void mf::Output::resource_destructor(wl_resource* resource)
{
    auto& map = *reinterpret_cast<decltype(resource_map)*>(
        wl_resource_get_user_data(resource));

    auto& client_resource_list = map[wl_resource_get_client(resource)];
    auto erase_from = std::remove_if(
        client_resource_list.begin(),
        client_resource_list.end(),
        [resource](auto candidate)
            { return candidate == resource; });
    client_resource_list.erase(erase_from, client_resource_list.end());
}


mf::OutputManager::OutputManager(wl_display* display, std::shared_ptr<MirDisplay> const& display_config) :
    display_config{display_config},
    display{display}
{
    // TODO: Also register display configuration listeners
    display_config->for_each_output(std::bind(&OutputManager::create_output, this, std::placeholders::_1));
}

auto mf::OutputManager::output_id_for(
    wl_client* client,
    std::experimental::optional<struct wl_resource*> const& output) const
    -> mir::optional_value<graphics::DisplayConfigurationOutputId>
{
    if (output)
    {
        return output_id_for(client, output.value());
    }
    else
    {
        return {};
    }
}

auto mf::OutputManager::output_id_for(wl_client* client, struct wl_resource* output) const
    -> mir::graphics::DisplayConfigurationOutputId
{
    for (auto const& dd: outputs)
        if (dd.second->matches_client_resource(client, output))
            return {dd.first};

    return {};
}

void mf::OutputManager::create_output(mg::DisplayConfigurationOutput const& initial_config)
{
    if (initial_config.used)
    {
        outputs.emplace(initial_config.id, std::make_unique<Output>(display, initial_config));
    }
}

void mf::OutputManager::handle_configuration_change(mg::DisplayConfiguration const& config)
{
    config.for_each_output([this](mg::DisplayConfigurationOutput const& output_config)
        {
            auto output_iter = outputs.find(output_config.id);
            if (output_iter != outputs.end())
            {
                if (output_config.used)
                {
                    output_iter->second->handle_configuration_changed(output_config);
                }
                else
                {
                    outputs.erase(output_iter);
                }
            }
            else if (output_config.used)
            {
                outputs[output_config.id] = std::make_unique<Output>(display, output_config);
            }
        });
}
