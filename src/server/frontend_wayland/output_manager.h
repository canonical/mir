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
#include "weak.h"
#include "client.h"

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

class OutputInstance : public wayland_rs::WlOutputImpl, OutputConfigListener, public std::enable_shared_from_this<OutputInstance>
{
public:
    OutputInstance(OutputGlobal* global, std::shared_ptr<wayland_rs::Client> const& client);
    ~OutputInstance();

    static auto from(WlOutputImpl* impl) -> OutputInstance*;

    auto output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool override;
    void send_done();

    wayland_rs::Weak<OutputGlobal> const global;
    std::shared_ptr<wayland_rs::Client> const client;
};

class OutputGlobal: public wayland_rs::LifetimeTracker, public std::enable_shared_from_this<OutputGlobal>
{
public:
    OutputGlobal(graphics::DisplayConfigurationOutput const& initial_configuration);

    static auto from(wayland_rs::WlOutputImpl* impl) -> OutputGlobal*;
    static auto from_or_throw(wayland_rs::WlOutputImpl* impl) -> OutputGlobal&;

    void handle_configuration_changed(graphics::DisplayConfigurationOutput const& config);
    void for_each_output_bound_by(wayland_rs::Client* client, std::function<void(OutputInstance*)> const& functor);
    auto current_config() -> graphics::DisplayConfigurationOutput const& { return output_config; }

    void add_listener(OutputConfigListener* listener);
    void remove_listener(OutputConfigListener* listener);
    std::shared_ptr<OutputInstance> create(std::shared_ptr<wayland_rs::Client> const& client);

private:
    friend OutputInstance;

    void instance_destroyed(OutputInstance* instance);

    graphics::DisplayConfigurationOutput output_config;
    std::vector<OutputConfigListener*> listeners;
    std::unordered_map<wayland_rs::Client*, std::vector<wayland_rs::Weak<OutputInstance>>> instances;
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
        std::shared_ptr<Executor> const& executor,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& registrar);
    ~OutputManager();

    static auto output_id_for(wayland_rs::WlOutputImpl* impl)
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

    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const registrar;
    std::shared_ptr<DisplayConfigObserver> const display_config_observer;
    std::unordered_map<graphics::DisplayConfigurationOutputId, std::unique_ptr<OutputGlobal>> outputs;
    std::shared_ptr<graphics::DisplayConfiguration const> display_config;
    std::vector<OutputManagerListener*> listeners;
};
}
}

#endif //MIR_FRONTEND_OUTPUT_MANAGER_H_
