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
    ObserverRegistrar<mg::DisplayConfigurationObserver>& display_configuration_observers,
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<Executor> const& executor,
    std::shared_ptr<input::CursorImages> const& cursor_images) :
    buffer_allocator{buffer_allocator},
    executor{executor},
    cursor_images{cursor_images},
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

    std::unique_lock lock{mutex};
    if (decorations.find(surface.get()) == decorations.end())
    {
        decorations[surface.get()] = nullptr;
        lock.unlock();
        auto decoration =
            std::make_unique<BasicDecoration>(locked_shell, buffer_allocator, executor, cursor_images, surface);
        lock.lock();
        decoration->set_scale(scale);
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

void msd::BasicManager::set_scale(float new_scale)
{
    std::lock_guard lock{mutex};
    if (new_scale != scale)
    {
        scale = new_scale;
        for (auto& it : decorations)
        {
            it.second->set_scale(scale);
        }
    }
}
