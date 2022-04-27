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

#include "output_manager.h"
#include "wayland_executor.h"

#include <mir/frontend/display_changer.h>
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

auto transform_size(Size const& size, MirOrientation orientation) -> Size
{
    switch (orientation)
    {
    case mir_orientation_normal:
    case mir_orientation_inverted:
        return size;

    case mir_orientation_left:
    case mir_orientation_right:
        return {size.height.as_uint32_t(), size.width.as_uint32_t()};
    }

    return size;
}
}

mf::OutputInstance::OutputInstance(wl_resource* resource, OutputGlobal* global)
    : Output{resource, Version<3>()},
      global{mw::make_weak(global)}
{
}

mf::OutputInstance::~OutputInstance()
{
    if (global)
    {
        global.value().instance_destroyed(this);
    }
}

void mf::OutputInstance::send_config(mg::DisplayConfigurationOutput const& config)
{
    // TODO: send correct output transform
    //       this will cause some clients to send transformed buffers, which we must be able to deal with
    send_geometry_event(
        config.top_left.x.as_int(),
        config.top_left.y.as_int(),
        config.physical_size_mm.width.as_int(),
        config.physical_size_mm.height.as_int(),
        as_subpixel_arrangement(config.subpixel_arrangement),
        "Fake manufacturer",
        "Fake model",
        mw::Output::Transform::normal);
    for (size_t i = 0; i < config.modes.size(); ++i)
    {
        auto const& mode = config.modes[i];

        // As we are not sending the display orientation as a transform (see above),
        // we doctor the size of each mode to match the extents
        auto const size = transform_size(mode.size, config.orientation);
        send_mode_event(
            ((i == config.preferred_mode_index ? mw::Output::Mode::preferred : 0) |
             (i == config.current_mode_index ? mw::Output::Mode::current : 0)),
             size.width.as_int(),
             size.height.as_int(),
            mode.vrefresh_hz * 1000);
    }

    if (version_supports_scale())
    {
        send_scale_event(ceil(config.scale));
    }

    if (version_supports_done())
    {
        send_done_event();
    }
}

mf::OutputGlobal::OutputGlobal(wl_display* display, mg::DisplayConfigurationOutput const& initial_configuration)
    : Global{display, Version<3>{}},
      current_config{initial_configuration}
{
}

void mf::OutputGlobal::handle_configuration_changed(mg::DisplayConfigurationOutput const& config)
{
    for (auto const& client : instances)
    {
        for (auto const& instance : client.second)
        {
            instance->send_config(config);
        }
    }
}

void mf::OutputGlobal::for_each_output_bound_by(
    wl_client* client,
    std::function<void(wayland::Output*)> const& functor)
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

void mf::OutputGlobal::bind(wl_resource* resource)
{
    auto const instance = new OutputInstance(resource, this);
    instances[wl_resource_get_client(resource)].push_back(instance);
    instance->send_config(current_config);
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

auto mf::OutputManager::output_id_for(wl_client* client, wl_resource* output) const
    -> std::optional<graphics::DisplayConfigurationOutputId>
{
    if (!output)
    {
        return std::nullopt;
    }

    for (auto const& dd : outputs)
    {
        bool found{false};
        dd.second->for_each_output_bound_by(client, [&found, &output](wayland::Output* candidate)
            {
                if (candidate->resource == output)
                {
                    found = true;
                }
            });
        if (found)
            return dd.first;
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

auto mf::OutputManager::with_config_for(
    wl_resource* output,
    std::function<void(mg::DisplayConfigurationOutput const&)> const& functor) -> bool
{
    bool found = false;
    if (auto const output_id = output_id_for(wl_resource_get_client(output), output))
    {
        for_each_output(
            [&](mg::DisplayConfigurationOutput const& config)
            {
                if (config.id == output_id.value())
                {
                    if (found)
                    {
                        log_warning("Found multiple output configs with id %d", output_id.value().as_value());
                    }
                    else
                    {
                        functor(config);
                        found = true;
                    }
                }
            });
    }

    return found;
}

void mf::OutputManager::for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const
{
    if (current_config)
    {
        current_config->for_each_output(f);
    }
    else
    {
        fatal_error("OutputManager::for_each_output() can't run because it doesn't have a display config yet");
    }
}

void mf::OutputManager::handle_configuration_change(std::shared_ptr<mg::DisplayConfiguration const> const& config)
{
    current_config = config;
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
                outputs[output_config.id] = std::make_unique<OutputGlobal>(display, output_config);
            }
        });
}
