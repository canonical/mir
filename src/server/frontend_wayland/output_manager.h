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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_OUTPUT_MANAGER_H_
#define MIR_FRONTEND_OUTPUT_MANAGER_H_

#include <mir/graphics/display_configuration.h>

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include <experimental/optional>

#include <memory>
#include <vector>
#include <unordered_map>

namespace mir
{
namespace frontend
{
class DisplayChanger;

class Output
{
public:
    Output(wl_display* display, graphics::DisplayConfigurationOutput const& initial_configuration);

    ~Output();

    void handle_configuration_changed(graphics::DisplayConfigurationOutput const& /*config*/);

    bool matches_client_resource(wl_client* client, struct wl_resource* resource) const;

private:
    static void send_initial_config(wl_resource* client_resource, graphics::DisplayConfigurationOutput const& config);

    wl_global* make_output(wl_display* display);

    static void on_bind(wl_client* client, void* data, uint32_t version, uint32_t id);

    static void resource_destructor(wl_resource* resource);

private:
    wl_global* const output;
    graphics::DisplayConfigurationOutput current_config;
    std::unordered_map<wl_client*, std::vector<wl_resource*>> resource_map;
};

class OutputManager
{
public:
    OutputManager(wl_display* display, DisplayChanger& display_config);

    auto output_id_for(wl_client* client, std::experimental::optional<struct wl_resource*> const& /*output*/) const
        -> optional_value<graphics::DisplayConfigurationOutputId>;

    auto output_id_for(wl_client* client, struct wl_resource* /*output*/) const
        -> graphics::DisplayConfigurationOutputId;

private:
    void create_output(graphics::DisplayConfigurationOutput const& initial_config);

    void handle_configuration_change(graphics::DisplayConfiguration const& config);

    wl_display* const display;
    std::unordered_map<graphics::DisplayConfigurationOutputId, std::unique_ptr<Output>> outputs;
};
}
}

#endif //MIR_FRONTEND_OUTPUT_MANAGER_H_
