/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "basic_manager.h"
#include "decoration.h"

#include <mir/graphics/display_configuration_observer.h>
#include <mir/graphics/display_configuration.h>

#include <boost/throw_exception.hpp>
#include <vector>

namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;

class msd::DisplayConfigurationListener :  public mg::DisplayConfigurationObserver
{
public:
    void set_listener(OutputListener* listener)
    {
        output_listener = listener;
    }

private:
    void initial_configuration(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        output_listener->advise_output_update(*config);
    }
    void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        output_listener->advise_output_update(*config);
    }

    void configuration_failed(
        std::shared_ptr<mg::DisplayConfiguration const /*attempted*/> const&,
        std::exception const& /*error*/) override {}

    void catastrophic_configuration_error(
        std::shared_ptr<mg::DisplayConfiguration const /*failed_fallback*/> const&,
        std::exception const& /*error*/) override {}

    void base_configuration_updated(
        std::shared_ptr<mg::DisplayConfiguration const> const& /*base_config*/) override {}

    void session_configuration_applied(
        std::shared_ptr<ms::Session> const& /*session*/,
        std::shared_ptr<mg::DisplayConfiguration> const& /*config*/) override {}

    void session_configuration_removed(std::shared_ptr<ms::Session> const& /*session*/) override {}

    void configuration_updated_for_session(
        std::shared_ptr<ms::Session> const& /*session*/,
        std::shared_ptr<mg::DisplayConfiguration const> const& /*config*/) override {}

    OutputListener* output_listener;
};

msd::BasicManager::BasicManager(
    ObserverRegistrar<mg::DisplayConfigurationObserver>& display_configuration_observers,
    DecorationBuilder&& decoration_builder) :
    decoration_builder{decoration_builder},
    display_config_monitor{std::make_shared<DisplayConfigurationListener>()}
{
    display_config_monitor->set_listener(this);
    display_configuration_observers.register_interest(display_config_monitor);
}

msd::BasicManager::~BasicManager()
{
    undecorate_all();
}

void msd::BasicManager::init(std::weak_ptr<shell::Shell> const& shell_)
{
    shell = shell_;
}

void msd::BasicManager::decorate(std::shared_ptr<ms::Surface> const& surface)
{
    auto const locked_shell = shell.lock();
    if (!locked_shell)
        BOOST_THROW_EXCEPTION(std::runtime_error("Shell is null"));

    std::unique_lock lock{mutex};
    if (decorations.find(surface.get()) == decorations.end())
    {
        decorations[surface.get()] = nullptr;
        lock.unlock();
        auto decoration = decoration_builder(locked_shell, surface);
        if (decoration)
        {
            decoration->set_scale(scale);
        }
        lock.lock();
        decorations[surface.get()] = std::move(decoration);
    }
}

void msd::BasicManager::undecorate(std::shared_ptr<ms::Surface> const& surface)
{
    std::unique_ptr<Decoration> decoration;
    {
        std::lock_guard lock{mutex};
        auto const it = decorations.find(surface.get());
        if (it != decorations.end())
        {
            decoration = std::move(it->second);
            decorations.erase(it);
        }
    }
    // Destroy the decoration outside the lock
    decoration.reset();
}

void msd::BasicManager::undecorate_all()
{
    std::vector<std::unique_ptr<Decoration>> to_destroy;
    {
        std::lock_guard lock{mutex};
        for (auto& it : decorations)
            to_destroy.push_back(std::move(it.second));
        decorations.clear();
    }
    // Destroy the decorations outside the lock
    to_destroy.clear();
}

void msd::BasicManager::advise_output_update(mg::DisplayConfiguration const& config)
{
    // Use the maximum scale to ensure sharp-looking decorations on all outputs
    auto max_output_scale{0.0f};
    config.for_each_output(
        [&](mg::DisplayConfigurationOutput const& output)
        {
            if (!output.used || !output.connected) return;
            if (!output.valid() || (output.current_mode_index >= output.modes.size())) return;

            max_output_scale = std::max(max_output_scale, output.scale);
        });

    if (max_output_scale == scale) return;

    scale = max_output_scale;

    std::lock_guard lock{mutex};
    for (auto& it : decorations)
    {
        if (it.second)
        {
            it.second->set_scale(scale);
        }
    }
}