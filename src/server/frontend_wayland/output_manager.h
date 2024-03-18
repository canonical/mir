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

#ifndef MIR_FRONTEND_OUTPUT_MANAGER_H_
#define MIR_FRONTEND_OUTPUT_MANAGER_H_

#include <mir/graphics/display_configuration.h>
#include <mir/graphics/null_display_configuration_observer.h>

#include "wayland_wrapper.h"
#include "mir/wayland/weak.h"

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
class OutputGlobal;

class OutputConfigListener
{
public:
    // Returns true if the wl_output needs to send a done event
    virtual auto output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool = 0;

    OutputConfigListener() = default;
    virtual ~OutputConfigListener() = default;
    OutputConfigListener(OutputConfigListener const&) = delete;
    OutputConfigListener& operator=(OutputConfigListener const&) = delete;
};

class OutputInstance : public wayland::Output, OutputConfigListener
{
public:
    OutputInstance(wl_resource* resource, OutputGlobal* global);
    ~OutputInstance();

    static auto from(wl_resource* output) -> OutputInstance*;

    auto output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool override;
    void send_done();

    wayland::Weak<OutputGlobal> const global;
};

class OutputGlobal: public wayland::LifetimeTracker, wayland::Output::Global
{
public:
    OutputGlobal(wl_display* display, graphics::DisplayConfigurationOutput const& initial_configuration);

    static auto from(wl_resource* output) -> OutputGlobal*;
    static auto from_or_throw(wl_resource* output) -> OutputGlobal&;

    void handle_configuration_changed(graphics::DisplayConfigurationOutput const& config);
    void for_each_output_bound_by(wayland::Client* client, std::function<void(OutputInstance*)> const& functor);
    auto current_config() -> graphics::DisplayConfigurationOutput const& { return output_config; }

    void add_listener(OutputConfigListener* listener);
    void remove_listener(OutputConfigListener* listener);

private:
    friend OutputInstance;

    void bind(wl_resource* resource) override;
    void instance_destroyed(OutputInstance* instance);

    graphics::DisplayConfigurationOutput output_config;
    std::vector<OutputConfigListener*> listeners;
    std::unordered_map<wayland::Client*, std::vector<OutputInstance*>> instances;
};

class OutputManagerListener
{
public:
    virtual void output_global_created(OutputGlobal* global) = 0;

    OutputManagerListener() = default;
    virtual ~OutputManagerListener() = default;
    OutputManagerListener(OutputManagerListener const&) = delete;
    OutputManagerListener& operator=(OutputManagerListener const&) = delete;
};

class OutputManager
{
public:
    OutputManager(
        wl_display* display,
        std::shared_ptr<Executor> const& executor,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& registrar);
    ~OutputManager();

    static auto output_id_for(std::optional<wl_resource*> output)
        -> std::optional<graphics::DisplayConfigurationOutputId>;

    auto output_for(graphics::DisplayConfigurationOutputId id) -> std::optional<OutputGlobal*>;
    auto current_config() -> graphics::DisplayConfiguration const& { return *display_config; }

    void add_listener(OutputManagerListener* listener);
    void remove_listener(OutputManagerListener* listener);

private:
    void handle_configuration_change(std::shared_ptr<graphics::DisplayConfiguration const> const& config);

    struct DisplayConfigObserver;

    wl_display* const display;
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const registrar;
    std::shared_ptr<DisplayConfigObserver> const display_config_observer;
    std::unordered_map<graphics::DisplayConfigurationOutputId, std::unique_ptr<OutputGlobal>> outputs;
    std::shared_ptr<graphics::DisplayConfiguration const> display_config;
    std::vector<OutputManagerListener*> listeners;
};
}
}

#endif //MIR_FRONTEND_OUTPUT_MANAGER_H_
