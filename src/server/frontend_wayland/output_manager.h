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

#include "wayland.h"
#include "client.h"
#include <mir/wayland/weak.h>
#include <mir/wayland/lifetime_tracker.h>

#include <rust/cxx.h>

#include <optional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <tuple>

namespace mir
{
template<typename T>
class ObserverRegistrar;
class Executor;

namespace wayland_rs
{
class WaylandServer;
class WaylandClientRegistry;
// The RAII handle for a dynamically-created `wl_output` global, returned by
// `WaylandServer::create_output_global`. Defined by the cxx bridge
// (`wayland_rs/src/ffi.rs.h`); forward-declared here so headers stay light.
struct OutputGlobal;
}

namespace frontend
{
class DisplayChanger;
class OutputGlobal;

class OutputConfigListener : public virtual wayland::LifetimeTracker
{
public:
    // Returns true if the wl_output needs to send a done event
    virtual auto output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool = 0;

    OutputConfigListener() = default;
    virtual ~OutputConfigListener() = default;
    OutputConfigListener(OutputConfigListener const&) = delete;
    OutputConfigListener& operator=(OutputConfigListener const&) = delete;
};

class OutputInstance : public wayland_rs::Output, public OutputConfigListener
{
public:
    OutputInstance(
        std::shared_ptr<wayland_rs::Client> const& client,
        rust::Box<wayland_rs::OutputMiddleware> instance,
        uint32_t object_id,
        OutputGlobal* global);
    ~OutputInstance();

    static auto from(wayland_rs::Output* output) -> OutputInstance*;

    auto output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool override;
    void send_done();

    // Disambiguate the two inherited LifetimeTracker::destroyed_flag() (from
    // wayland_rs::Output and from OutputConfigListener).
    using wayland_rs::Output::destroyed_flag;

    wayland::Weak<OutputGlobal> const global;
};

class OutputGlobal : public wayland::LifetimeTracker
{
public:
    OutputGlobal(
        wayland_rs::WaylandServer& server,
        wayland_rs::WaylandClientRegistry& registry,
        graphics::DisplayConfigurationOutput const& initial_configuration);
    ~OutputGlobal();

    static auto from(wayland_rs::Output* output) -> OutputGlobal*;
    static auto from_or_throw(wayland_rs::Output* output) -> OutputGlobal&;

    void handle_configuration_changed(graphics::DisplayConfigurationOutput const& config);
    void for_each_output_bound_by(wayland_rs::Client* client, std::function<void(OutputInstance*)> const& functor);
    auto current_config() -> graphics::DisplayConfigurationOutput const& { return output_config; }

    void add_listener(OutputConfigListener* listener);
    void remove_listener(OutputConfigListener* listener);

    // Called by the per-monitor binder when a client binds this monitor's
    // `wl_output` global. Constructs the per-bind `OutputInstance` (whose
    // ownership passes to the Rust server via the returned shared_ptr).
    auto bind(
        std::shared_ptr<wayland_rs::Client> const& client,
        rust::Box<wayland_rs::OutputMiddleware> instance,
        uint32_t object_id) -> std::shared_ptr<wayland_rs::Output>;

private:
    friend OutputInstance;

    void instance_destroyed(OutputInstance* instance);

    graphics::DisplayConfigurationOutput output_config;
    std::vector<wayland::Weak<OutputConfigListener>> listeners;
    std::unordered_map<wayland_rs::Client*, std::vector<OutputInstance*>> instances;
    // Owns the wl_output global's lifetime: destroyed with this OutputGlobal,
    // which withdraws the global from the display.
    std::optional<rust::Box<wayland_rs::OutputGlobal>> global_handle;
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
        wayland_rs::WaylandServer& server,
        wayland_rs::WaylandClientRegistry& registry,
        std::shared_ptr<Executor> const& executor,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& registrar);
    ~OutputManager();

    static auto output_id_for(wayland_rs::Output* output)
        -> std::optional<graphics::DisplayConfigurationOutputId>;

    auto output_for(graphics::DisplayConfigurationOutputId id) -> std::optional<OutputGlobal*>;
    auto current_config() -> graphics::DisplayConfiguration const& { return *display_config; }

    void add_listener(OutputManagerListener* listener);
    void remove_listener(OutputManagerListener* listener);

    static auto from_output_transform(int32_t transform) -> std::tuple<MirOrientation, MirMirrorMode>;
    static auto to_output_transform(MirOrientation orientation, MirMirrorMode mirror_mode) -> int32_t;
private:
    void handle_configuration_change(std::shared_ptr<graphics::DisplayConfiguration const> const& config);

    struct DisplayConfigObserver;

    wayland_rs::WaylandServer& server;
    wayland_rs::WaylandClientRegistry& registry;
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const registrar;
    std::shared_ptr<DisplayConfigObserver> const display_config_observer;
    std::unordered_map<graphics::DisplayConfigurationOutputId, std::unique_ptr<OutputGlobal>> outputs;
    std::shared_ptr<graphics::DisplayConfiguration const> display_config;
    std::vector<OutputManagerListener*> listeners;
};
}
}

#endif //MIR_FRONTEND_OUTPUT_MANAGER_H_
