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

#include "mir_display.h"

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
class MirDisplay;

class Output
{
public:
    Output(wl_display* display, graphics::DisplayConfigurationOutput const& initial_configuration);

    ~Output();

    void handle_configuration_changed(graphics::DisplayConfigurationOutput const& /*config*/);

    void for_each_output_resource_bound_by(wl_client* client, std::function<void(wl_resource*)> const& functor);

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

class OutputManager : public OutputObserver
{
public:
    OutputManager(wl_display* display, std::shared_ptr<MirDisplay> const& display_config, std::shared_ptr<Executor> const& executor);
    ~OutputManager();

    auto output_id_for(wl_client* client, wl_resource* output) const
        -> std::experimental::optional<graphics::DisplayConfigurationOutputId>;

    auto output_for(graphics::DisplayConfigurationOutputId id) -> std::experimental::optional<Output*>;

    auto display_config() const -> std::shared_ptr<MirDisplay> {return display_config_;}

private:
    void create_output(graphics::DisplayConfigurationOutput const& initial_config);

    void handle_configuration_change(graphics::DisplayConfiguration const& config) override;

    std::shared_ptr<MirDisplay> const display_config_;
    wl_display* const display;
    std::shared_ptr<Executor> const executor;
    std::unordered_map<graphics::DisplayConfigurationOutputId, std::unique_ptr<Output>> outputs;
};
}
}

#endif //MIR_FRONTEND_OUTPUT_MANAGER_H_
