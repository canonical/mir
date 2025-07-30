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
#include "wayland_executor.h"

#include <mir/observer_registrar.h>
#include <mir/log.h>

#include <algorithm>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mw = mir::wayland;
using namespace mir::geometry;

namespace
{
auto as_subpixel_arrangement(MirSubpixelArrangement arrangement) -> uint32_t
{
    switch (arrangement)
    {
    default:
    case mir_subpixel_arrangement_unknown:
        return mw::Output::Subpixel::unknown;

    case mir_subpixel_arrangement_horizontal_rgb:
        return mw::Output::Subpixel::horizontal_rgb;

    case mir_subpixel_arrangement_horizontal_bgr:
        return mw::Output::Subpixel::horizontal_bgr;

    case mir_subpixel_arrangement_vertical_rgb:
        return mw::Output::Subpixel::vertical_rgb;

    case mir_subpixel_arrangement_vertical_bgr:
        return mw::Output::Subpixel::vertical_bgr;

    case mir_subpixel_arrangement_none:
        return mw::Output::Subpixel::none;
    }
}

}

mf::OutputInstance::OutputInstance(wl_resource* resource, OutputGlobal* global)
    : Output{resource, Version<4>()},
      global{mw::make_weak(global)}
{
    global->add_listener(this);
    send_name_event_if_supported(global->output_config.name);
}

mf::OutputInstance::~OutputInstance()
{
    if (global)
    {
        global.value().instance_destroyed(this);
        global.value().remove_listener(this);
    }
}

auto mf::OutputInstance::from(wl_resource* output) -> OutputInstance*
{
    return dynamic_cast<OutputInstance*>(mw::Output::from(output));
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
            ((i == config.preferred_mode_index ? mw::Output::Mode::preferred : 0) |
             (i == config.current_mode_index ? mw::Output::Mode::current : 0)),
             mode.size.width.as_int(),
             mode.size.height.as_int(),
            mode.vrefresh_hz * 1000);
    }

    if (version_supports_scale())
    {
        send_scale_event(ceil(config.scale));
    }

    return true;
}

void mf::OutputInstance::send_done()
{
    if (version_supports_done())
    {
        send_done_event();
    }
}

mf::OutputGlobal::OutputGlobal(wl_display* display, mg::DisplayConfigurationOutput const& initial_configuration)
    : Global{display, Version<4>{}},
      output_config{initial_configuration}
{
}

auto mf::OutputGlobal::from(wl_resource* output) -> OutputGlobal*
{
    auto const instance = OutputInstance::from(output);
    return instance ? mw::as_nullable_ptr(instance->global) : nullptr;
}

auto mf::OutputGlobal::from_or_throw(wl_resource* output) -> OutputGlobal&
{
    auto const instance = OutputInstance::from(output);
    if (!instance)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "wl_output@" +
            std::to_string(wl_resource_get_id(output)) +
            " is not a mir::frontend::OutputInstance"));
    }
    if (!instance->global)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "the output global associated with wl_output@" +
            std::to_string(wl_resource_get_id(output)) +
            " has been destroyed"));
    }
    return instance->global.value();
}

void mf::OutputGlobal::handle_configuration_changed(mg::DisplayConfigurationOutput const& config)
{
    output_config = config;
    bool needs_done = false;
    for (auto& listener : listeners)
    {
        if (listener->output_config_changed(config))
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
    wayland::Client* client,
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
    listeners.push_back(listener);
}

void mf::OutputGlobal::remove_listener(OutputConfigListener* listener)
{
    listeners.erase(
        std::remove_if(listeners.begin(), listeners.end(), [&](auto candidate) { return candidate == listener; }),
        listeners.end());
}

void mf::OutputGlobal::bind(wl_resource* resource)
{
    auto const instance = new OutputInstance(resource, this);
    instances[instance->client].push_back(instance);
    for (auto const& listener : listeners)
    {
        listener->output_config_changed(output_config);
    }
    instance->send_done();
}

void mf::OutputGlobal::instance_destroyed(OutputInstance* instance)
{
    auto const iter = instances.find(instance->client);
    if (iter == instances.end())
    {
        return;
    }
    auto& vec = iter->second;
    vec.erase(
        std::remove_if(vec.begin(), vec.end(), [&](auto candidate) { return candidate == instance; }),
        vec.end());
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
    wl_display* display,
    std::shared_ptr<Executor> const& executor,
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& registrar)
    : display{display},
      registrar{registrar},
      display_config_observer{std::make_shared<DisplayConfigObserver>(*this, executor)}
{
    registrar->register_interest(display_config_observer, *executor);
}

mf::OutputManager::~OutputManager()
{
    registrar->unregister_interest(*display_config_observer);
}

auto mf::OutputManager::output_id_for(std::optional<wl_resource*> output)
    -> std::optional<graphics::DisplayConfigurationOutputId>
{
    if (output)
    {
        if (auto const global = OutputGlobal::from(output.value()))
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
        case mw::Output::Transform::normal:
            break;
        case mw::Output::Transform::_90:
            orientation = mir_orientation_left;
            break;
        case mw::Output::Transform::_180:
            orientation = mir_orientation_inverted;
            break;
        case mw::Output::Transform::_270:
            orientation = mir_orientation_right;
            break;
        case mw::Output::Transform::flipped:
            orientation = mir_orientation_normal;
            mirror_mode = mir_mirror_mode_horizontal;
            break;
        case mw::Output::Transform::flipped_90:
            orientation = mir_orientation_left;
            mirror_mode = mir_mirror_mode_horizontal;
            break;
        case mw::Output::Transform::flipped_180:
            orientation = mir_orientation_inverted;
            mirror_mode = mir_mirror_mode_horizontal;
            break;
        case mw::Output::Transform::flipped_270:
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
            return mw::Output::Transform::normal;
    }

    // Lookup table: [orientation_index][mirror_mode]
    static constexpr int32_t transform_table[4][3] = {
        // mir_orientation_normal
        { mw::Output::Transform::normal, mw::Output::Transform::normal, mw::Output::Transform::flipped },
        // mir_orientation_left
        { mw::Output::Transform::_90, mw::Output::Transform::_90, mw::Output::Transform::flipped_90 },
        // mir_orientation_inverted
        { mw::Output::Transform::_180, mw::Output::Transform::_180, mw::Output::Transform::flipped_180 },
        // mir_orientation_right
        { mw::Output::Transform::_270, mw::Output::Transform::_270, mw::Output::Transform::flipped_270 },
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
                auto output_global{std::make_unique<OutputGlobal>(display, output_config)};
                auto* output_global_pointer{output_global.get()};
                outputs[output_config.id] = std::move(output_global);
                for (auto& listener : listeners)
                {
                    listener->output_global_created(output_global_pointer);
                }
            }
        });
}
