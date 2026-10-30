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
#include "decoration_strategy.h"

#include <mir/graphics/display_configuration.h>
#include <mir/graphics/null_display_configuration_observer.h>

#include <boost/throw_exception.hpp>
#include <vector>

namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;

class msd::DisplayConfigurationListener :  public mg::NullDisplayConfigurationObserver
{
public:
    using Callback = std::function<void(mg::DisplayConfiguration const&)>;

    explicit DisplayConfigurationListener(Callback callback) : callback{std::move(callback)} {}

private:
    void initial_configuration(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        callback(*config);
    }
    void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        callback(*config);
    }

    Callback const callback;
};

msd::BasicManager::BasicManager(
    std::shared_ptr<DecorationStrategy> const& decoration_strategy,
    ObserverRegistrar<mg::DisplayConfigurationObserver>& display_configuration_observers,
    DecorationBuilder&& decoration_builder) :
    decoration_strategy{decoration_strategy},
    decoration_builder{std::move(decoration_builder)},
    display_config_monitor{std::make_shared<DisplayConfigurationListener>(
        [&](mg::DisplayConfiguration const& config)
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
            set_scale(max_output_scale);
        })}
{
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

    auto const token = std::make_shared<BuildToken>();
    {
        std::lock_guard lock{mutex};
        if (decorations.find(surface.get()) != decorations.end())
            return;

        decorations.emplace(surface.get(), DecoratedSurface{surface, nullptr, token});
    }
    build_decoration(surface, locked_shell, token);
}

void msd::BasicManager::build_decoration(
    std::shared_ptr<ms::Surface> const& surface,
    std::shared_ptr<msh::Shell> const& locked_shell,
    std::shared_ptr<BuildToken> const& token)
{
    while (true)
    {
        {
            std::lock_guard lock{mutex};
            auto const it = decorations.find(surface.get());
            if (it == decorations.end() || it->second.building != token)
                return;
        }

        auto decoration = decoration_builder(decoration_strategy, locked_shell, surface);
        bool rebuild = false;
        while (!rebuild)
        {
            float current_scale;
            {
                std::lock_guard lock{mutex};
                auto const it = decorations.find(surface.get());
                if (it == decorations.end() || it->second.building != token)
                    return;

                if (it->second.rebuild_pending)
                {
                    it->second.rebuild_pending = false;
                    rebuild = true;
                    continue;
                }

                current_scale = scale;
            }

            decoration->set_scale(current_scale);
            {
                std::lock_guard lock{mutex};
                auto const it = decorations.find(surface.get());
                if (it == decorations.end() || it->second.building != token)
                    return;

                if (it->second.rebuild_pending)
                {
                    it->second.rebuild_pending = false;
                    rebuild = true;
                }
                else if (scale == current_scale)
                {
                    it->second.decoration = std::move(decoration);
                    it->second.building.reset();
                    return;
                }
            }
        }

        decoration.reset();
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
            decoration = std::move(it->second.decoration);
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
            to_destroy.push_back(std::move(it.second.decoration));
        decorations.clear();
    }
    // Destroy the decorations outside the lock
    to_destroy.clear();
}

void msd::BasicManager::redecorate_all()
{
    struct Build
    {
        std::shared_ptr<ms::Surface> surface;
        std::shared_ptr<BuildToken> token;
    };

    auto const locked_shell = shell.lock();
    std::vector<Build> to_redecorate;
    std::vector<std::unique_ptr<Decoration>> to_destroy;
    {
        std::lock_guard lock{mutex};
        for (auto it = decorations.begin(); it != decorations.end();)
        {
            if (auto const surface = it->second.surface.lock())
            {
                if (it->second.building)
                {
                    it->second.rebuild_pending = true;
                }
                else
                {
                    to_destroy.push_back(std::move(it->second.decoration));
                    auto const token = std::make_shared<BuildToken>();
                    it->second.building = token;
                    it->second.rebuild_pending = false;
                    to_redecorate.push_back({surface, token});
                }
                ++it;
            }
            else
            {
                to_destroy.push_back(std::move(it->second.decoration));
                it = decorations.erase(it);
            }
        }
    }

    if (!to_redecorate.empty() && !locked_shell)
    {
        {
            std::lock_guard lock{mutex};
            for (auto const& [surface, token] : to_redecorate)
            {
                auto const it = decorations.find(surface.get());
                if (it != decorations.end() && it->second.building == token)
                    decorations.erase(it);
            }
        }
        to_destroy.clear();
        BOOST_THROW_EXCEPTION(std::runtime_error("Shell is null"));
    }

    // Destroy the decorations outside the lock
    to_destroy.clear();

    if (!to_redecorate.empty())
    {
        for (auto const& [surface, token] : to_redecorate)
            build_decoration(surface, locked_shell, token);
    }
}

using mir::geometry::Size;

auto msd::BasicManager::compute_size_with_decorations(Size content_size,
    MirWindowType type, MirWindowState state) -> Size
{
    return decoration_strategy->compute_size_with_decorations(content_size, type, state);
}

void msd::BasicManager::set_scale(float new_scale)
{
    std::lock_guard lock{mutex};
    if (new_scale != scale)
    {
        scale = new_scale;
        for (auto& it : decorations)
        {
            if (auto const& decoration = it.second.decoration)
                decoration->set_scale(scale);
        }
    }
}
