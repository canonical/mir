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
 */

#ifndef MIR_FRONTEND_OUTPUT_MANAGER_H_
#define MIR_FRONTEND_OUTPUT_MANAGER_H_

#include <mir/graphics/display_configuration.h>
#include <mir/graphics/null_display_configuration_observer.h>

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include <optional>
#include <memory>
#include <vector>
#include <unordered_map>

namespace mir
{
template<typename T>
class ObserverRegistrar;
class Executor;
namespace frontend
{
class DisplayChanger;

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

class OutputManager
{
public:
    OutputManager(
        wl_display* display,
        std::shared_ptr<Executor> const& executor,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& registrar);
    ~OutputManager();

    /// Returns the output ID (if any) associated with the given output resource. Output resource may be nullptr.
    auto output_id_for(wl_client* client, wl_resource* output) const
        -> std::optional<graphics::DisplayConfigurationOutputId>;

    auto output_for(graphics::DisplayConfigurationOutputId id) -> std::optional<Output*>;

    /// Either calls functor exactly once and returns true, or does not call functor and returns false if the output is
    /// not found
    auto with_config_for(
        wl_resource* output,
        std::function<void(graphics::DisplayConfigurationOutput const&)> const& functor) -> bool;

    void for_each_output(std::function<void(graphics::DisplayConfigurationOutput const&)> f) const;

private:
    void handle_configuration_change(std::shared_ptr<graphics::DisplayConfiguration const> const& config);

    struct DisplayConfigObserver;

    wl_display* const display;
    std::shared_ptr<Executor> const executor;
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const registrar;
    std::shared_ptr<DisplayConfigObserver> const display_config_observer;
    std::unordered_map<graphics::DisplayConfigurationOutputId, std::unique_ptr<Output>> outputs;
    std::shared_ptr<graphics::DisplayConfiguration const> current_config;
};
}
}

#endif //MIR_FRONTEND_OUTPUT_MANAGER_H_
