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

#include "output_manager.h"

#include "wayland_client_registry.h"
#include "output_global_binder.h"
#include "wayland_rs/src/ffi.rs.h"

#include <mir/observer_registrar.h>
#include <mir/log.h>

#include <boost/throw_exception.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mw = mir::wayland_rs;
namespace mwrs = mir::wayland_rs;
using namespace mir::geometry;

namespace
{
auto as_subpixel_arrangement(MirSubpixelArrangement arrangement) -> uint32_t
{
    switch (arrangement)
    {
    default:
    case mir_subpixel_arrangement_unknown:
        return mwrs::Output::Subpixel::unknown;

    case mir_subpixel_arrangement_horizontal_rgb:
        return mwrs::Output::Subpixel::horizontal_rgb;

    case mir_subpixel_arrangement_horizontal_bgr:
        return mwrs::Output::Subpixel::horizontal_bgr;

    case mir_subpixel_arrangement_vertical_rgb:
        return mwrs::Output::Subpixel::vertical_rgb;

    case mir_subpixel_arrangement_vertical_bgr:
        return mwrs::Output::Subpixel::vertical_bgr;

    case mir_subpixel_arrangement_none:
        return mwrs::Output::Subpixel::none;
    }
}

/// The per-monitor `wayland_rs::OutputGlobalBinder`.
///
/// One is created per `OutputGlobal` and handed to the Rust server via
/// `WaylandServer::create_output_global`; the server owns it for the lifetime
/// of the global. When a client binds this monitor's `wl_output` global the
/// server calls `bind()`, which resolves the raw client and forwards to the
/// owning `OutputGlobal`.
class OutputGlobalBinder : public mwrs::OutputGlobalBinder
{
public:
    OutputGlobalBinder(mf::OutputGlobal& global, mwrs::WaylandClientRegistry& registry)
        : global{mw::make_weak(&global)},
          registry{registry}
    {
    }

    auto bind(
        rust::Box<mwrs::WaylandClient> client,
        rust::Box<mwrs::OutputMiddleware> instance,
        uint32_t object_id) -> std::shared_ptr<mwrs::Output> override
    {
        auto const resolved = registry.from(client);
        if (!resolved)
        {
            throw std::logic_error{"wl_output bound for an unregistered client"};
        }
        if (!global)
        {
            throw std::logic_error{"wl_output bound after its monitor was removed"};
        }
        return global.value().bind(resolved, std::move(instance), object_id);
    }

    auto can_view(rust::Box<mwrs::WaylandClientId> /*client_id*/) -> bool override
    {
        // Outputs are core Wayland globals and are visible to every client for
        // as long as the monitor exists.
        return static_cast<bool>(global);
    }

private:
    mw::Weak<mf::OutputGlobal> const global;
    mwrs::WaylandClientRegistry& registry;
};
}

mf::OutputInstance::OutputInstance(
    std::shared_ptr<mwrs::Client> const& client,
    rust::Box<mwrs::OutputMiddleware> instance,
    uint32_t object_id,
    OutputGlobal* global)
    : Output{client, std::move(instance), object_id},
      global{mw::make_weak(global)}
{
    global->add_listener(this);
    send_name_event_if_supported(global->output_config.name);
}

// clang can't tell that .value() won't throw in this context
// NOLINTBEGIN(bugprone-exception-escape)
mf::OutputInstance::~OutputInstance()
{
    if (global)
    {
        global.value().instance_destroyed(this);
        global.value().remove_listener(this);
    }
}
// NOLINTEND(bugprone-exception-escape)

auto mf::OutputInstance::from(mwrs::Output* output) -> OutputInstance*
{
    return dynamic_cast<OutputInstance*>(output);
}

auto mf::OutputInstance::output_config_changed(mg::DisplayConfigurationOutput const& config) -> bool
{
    send_geometry_event(
        config.top_left.x.as_int(),
        config.top_left.y.as_int(),
        config.physical_size_mm.width.as_int(),
        config.physical_size_mm.height.as_int(),
        as_subpixel_arrangement(config.subpixel_arrangement),
        config.display_info.vendor.value_or("Unknown manufacturer"),
        config.display_info.model.value_or("Unknown model"),
        OutputManager::to_output_transform(config.orientation, mir_mirror_mode_none));

    for (size_t i = 0; i < config.modes.size(); ++i)
    {
        auto const& mode = config.modes[i];

        send_mode_event(
            ((i == config.preferred_mode_index ? mwrs::Output::Mode::preferred : 0) |
             (i == config.current_mode_index ? mwrs::Output::Mode::current : 0)),
             mode.size.width.as_int(),
             mode.size.height.as_int(),
            mode.vrefresh_hz * 1000);
    }

    send_scale_event_if_supported(static_cast<int32_t>(ceil(config.scale)));

    return true;
}

void mf::OutputInstance::send_done()
{
    send_done_event_if_supported();
}

mf::OutputGlobal::OutputGlobal(
    mwrs::WaylandServer& server,
    mwrs::WaylandClientRegistry& registry,
    mg::DisplayConfigurationOutput const& initial_configuration)
    : output_config{initial_configuration}
{
    global_handle.emplace(
        server.create_output_global(std::make_unique<OutputGlobalBinder>(*this, registry)));
}

mf::OutputGlobal::~OutputGlobal() = default;

auto mf::OutputGlobal::from(mwrs::Output* output) -> OutputGlobal*
{
    auto const instance = OutputInstance::from(output);
    return instance ? mw::as_nullable_ptr(instance->global) : nullptr;
}

auto mf::OutputGlobal::from_or_throw(mwrs::Output* output) -> OutputGlobal&
{
    auto const instance = OutputInstance::from(output);
    if (!instance)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "wl_output is not a mir::frontend::OutputInstance"));
    }
    if (!instance->global)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "the output global associated with the wl_output has been destroyed"));
    }
    return instance->global.value();
}

void mf::OutputGlobal::handle_configuration_changed(mg::DisplayConfigurationOutput const& config)
{
    output_config = config;
    bool needs_done = false;
    for (auto& listener : listeners)
    {
        if (listener && listener.value().output_config_changed(config))
        {
            needs_done = true;
        }
    }
    if (needs_done)
    {
        for (auto const& client : instances)
        {
            for (auto const& instance : client.second)
            {
                instance->send_done();
            }
        }
    }
}

void mf::OutputGlobal::for_each_output_bound_by(
    mwrs::Client* client,
    std::function<void(OutputInstance*)> const& functor)
{
    auto const resources = instances.find(client);

    if (resources != instances.end())
    {
        for (auto const& resource : resources->second)
        {
            functor(resource);
        }
    }
}

void mf::OutputGlobal::add_listener(OutputConfigListener* listener)
{
    listeners.emplace_back(listener);
}

void mf::OutputGlobal::remove_listener(OutputConfigListener* listener)
{
    std::erase_if(listeners, [&](auto candidate) { return !candidate || &candidate.value() == listener; });
}

auto mf::OutputGlobal::bind(
    std::shared_ptr<mwrs::Client> const& client,
    rust::Box<mwrs::OutputMiddleware> instance,
    uint32_t object_id) -> std::shared_ptr<mwrs::Output>
{
    auto const output_instance =
        std::make_shared<OutputInstance>(client, std::move(instance), object_id, this);
    instances[client.get()].push_back(output_instance.get());
    for (auto const& listener : listeners)
    {
        if (listener) listener.value().output_config_changed(output_config);
    }
    output_instance->send_done();
    return output_instance;
}

void mf::OutputGlobal::instance_destroyed(OutputInstance* instance)
{
    auto const iter = instances.find(instance->client.get());
    if (iter == instances.end())
    {
        return;
    }
    auto& vec = iter->second;
    std::erase_if(vec, [&](auto candidate) { return candidate == instance; });
    if (vec.empty())
    {
        instances.erase(iter);
    }
}

class mf::OutputManager::DisplayConfigObserver: public graphics::NullDisplayConfigurationObserver
{
public:
    DisplayConfigObserver(OutputManager& manager, std::shared_ptr<Executor> const& executor)
        : manager{manager},
          executor{executor}
    {
    }

private:
    void initial_configuration(std::shared_ptr<graphics::DisplayConfiguration const> const& config) override
    {
        manager.handle_configuration_change(config);
    }

    void configuration_applied(std::shared_ptr<graphics::DisplayConfiguration const> const& config) override
    {
        manager.handle_configuration_change(config);
    }

    OutputManager& manager;
    std::shared_ptr<Executor> const executor;
};

mf::OutputManager::OutputManager(
    mwrs::WaylandServer& server,
    mwrs::WaylandClientRegistry& registry,
    std::shared_ptr<Executor> const& executor,
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& registrar)
    : server{server},
      registry{registry},
      registrar{registrar},
      display_config_observer{std::make_shared<DisplayConfigObserver>(*this, executor)}
{
    registrar->register_interest(display_config_observer, *executor);
}

mf::OutputManager::~OutputManager()
{
    registrar->unregister_interest(*display_config_observer);
}

auto mf::OutputManager::output_id_for(mwrs::Output* output)
    -> std::optional<graphics::DisplayConfigurationOutputId>
{
    if (output)
    {
        if (auto const global = OutputGlobal::from(output))
        {
            return global->current_config().id;
        }
    }
    return std::nullopt;
}

auto mf::OutputManager::output_for(graphics::DisplayConfigurationOutputId id) -> std::optional<OutputGlobal*>
{
    auto const result = outputs.find(id);
    if (result != outputs.end())
        return result->second.get();
    else
        return std::nullopt;
}

void mf::OutputManager::add_listener(OutputManagerListener* listener)
{
    listeners.push_back(listener);

    for (auto const& [_, output_global] : outputs)
    {
        listener->output_global_created(output_global.get());
    }
}

void mf::OutputManager::remove_listener(OutputManagerListener* listener)
{
    std::erase_if(listeners, [&](auto candidate) { return candidate == listener; });
}

auto mf::OutputManager::from_output_transform(int32_t transform) -> std::tuple<MirOrientation, MirMirrorMode>
{
    MirOrientation orientation = mir_orientation_normal;
    MirMirrorMode mirror_mode = mir_mirror_mode_none;
    switch (transform)
    {
        case mwrs::Output::Transform::normal:
            break;
        case mwrs::Output::Transform::r_90:
            orientation = mir_orientation_left;
            break;
        case mwrs::Output::Transform::r_180:
            orientation = mir_orientation_inverted;
            break;
        case mwrs::Output::Transform::r_270:
            orientation = mir_orientation_right;
            break;
        case mwrs::Output::Transform::flipped:
            orientation = mir_orientation_normal;
            mirror_mode = mir_mirror_mode_horizontal;
            break;
        case mwrs::Output::Transform::flipped_90:
            orientation = mir_orientation_left;
            mirror_mode = mir_mirror_mode_horizontal;
            break;
        case mwrs::Output::Transform::flipped_180:
            orientation = mir_orientation_inverted;
            mirror_mode = mir_mirror_mode_horizontal;
            break;
        case mwrs::Output::Transform::flipped_270:
            orientation = mir_orientation_right;
            mirror_mode = mir_mirror_mode_horizontal;
            break;
        default:
            throw std::out_of_range("Unknown transform");
    }

    return std::make_tuple(orientation, mirror_mode);
}

auto mir::frontend::OutputManager::to_output_transform(MirOrientation orientation, MirMirrorMode mirror_mode) -> int32_t
{
    int orientation_index;
    switch (orientation)
    {
        case mir_orientation_normal:
            orientation_index = 0;
            break;
        case mir_orientation_left:
            orientation_index = 1;
            break;
        case mir_orientation_inverted:
            orientation_index = 2;
            break;
        case mir_orientation_right:
            orientation_index = 3;
            break;
        default:
            return mwrs::Output::Transform::normal;
    }

    // Lookup table: [orientation_index][mirror_mode]
    static constexpr int32_t transform_table[4][3] = {
        // mir_orientation_normal
        { mwrs::Output::Transform::normal, mwrs::Output::Transform::normal, mwrs::Output::Transform::flipped },
        // mir_orientation_left
        { mwrs::Output::Transform::r_90, mwrs::Output::Transform::r_90, mwrs::Output::Transform::flipped_90 },
        // mir_orientation_inverted
        { mwrs::Output::Transform::r_180, mwrs::Output::Transform::r_180, mwrs::Output::Transform::flipped_180 },
        // mir_orientation_right
        { mwrs::Output::Transform::r_270, mwrs::Output::Transform::r_270, mwrs::Output::Transform::flipped_270 },
    };

    return transform_table[orientation_index][mirror_mode];
}

void mf::OutputManager::handle_configuration_change(std::shared_ptr<mg::DisplayConfiguration const> const& config)
{
    display_config = config;
    config->for_each_output([this](mg::DisplayConfigurationOutput const& output_config)
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
                auto output_global{std::make_unique<OutputGlobal>(server, registry, output_config)};
                auto* output_global_pointer{output_global.get()};
                outputs[output_config.id] = std::move(output_global);
                for (auto& listener : listeners)
                {
                    listener->output_global_created(output_global_pointer);
                }
            }
        });
}
