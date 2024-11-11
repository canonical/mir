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

#include "input.h"
#include "user_decoration_example.h"
#include "user_decoration_manager_example.h"
#include "miral/decoration/decoration_adapter.h"

#include <mir/shell/decoration.h>
#include <mir/graphics/display_configuration.h>
#include <mir/graphics/null_display_configuration_observer.h>
/* #include <bffoost/throw_exception.hpp> */
#include "mirserver-internal/mir/observer_registrar.h"


#include <vector>
#include <mutex>

namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace msh = mir::shell;

namespace msd = mir::shell::decoration;

class DisplayConfigurationListener: public mg::NullDisplayConfigurationObserver
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

UserDecorationManagerExample::UserDecorationManagerExample(
    mir::ObserverRegistrar<mg::DisplayConfigurationObserver>& display_configuration_observers,
    std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<mir::Executor> const& executor,
    std::shared_ptr<mir::input::CursorImages> const& cursor_images) :
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

UserDecorationManagerExample::~UserDecorationManagerExample()
{
    undecorate_all();
}

void UserDecorationManagerExample::init(std::weak_ptr<msh::Shell> const& shell_)
{
    shell = shell_;
}

void UserDecorationManagerExample::decorate(std::shared_ptr<ms::Surface> const& surface)
{
    auto const locked_shell = shell.lock();
    if (!locked_shell)
        return; /* BOOST_THROW_EXCEPTION(std::runtime_error("Shell is null")); */

    std::unique_lock lock{mutex};
    if (decorations.find(surface.get()) == decorations.end())
    {
        decorations[surface.get()] = nullptr;
        lock.unlock();
        auto decoration = create_decoration(locked_shell, surface);
        lock.lock();
        decoration->set_scale(scale);
        decorations[surface.get()] = std::move(decoration);
    }
}

void UserDecorationManagerExample::undecorate(std::shared_ptr<ms::Surface> const& surface)
{
    std::unique_ptr<msd::Decoration> decoration;
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

void UserDecorationManagerExample::undecorate_all()
{
    std::vector<std::unique_ptr<msd::Decoration>> to_destroy;
    {
        std::lock_guard lock{mutex};
        for (auto& it : decorations)
            to_destroy.push_back(std::move(it.second));
        decorations.clear();
    }
    // Destroy the decorations outside the lock
    to_destroy.clear();
}

std::unique_ptr<msd::Decoration> UserDecorationManagerExample::create_decoration(
    std::shared_ptr<msh::Shell> const locked_shell, std::shared_ptr<mir::scene::Surface> surface)
{
    auto platinum = color(220, 220, 220);
    auto outer_space = color(72, 74, 71);
    auto outer_platinum = color((220+72)/2, (220+74)/2, (220+71)/2);
    auto decoration = std::make_shared<UserDecorationExample>(
        locked_shell,
        buffer_allocator,
        executor,
        cursor_images,
        surface,
        Theme{platinum, outer_space},
        Theme{outer_space, platinum},
        ButtonTheme{
            {ButtonFunction::Close, Icon{color(233, 79, 55), color(253, 99, 75), platinum}},
            {ButtonFunction::Maximize, Icon{outer_space, outer_platinum, platinum}},
            {ButtonFunction::Minimize, Icon{outer_space, outer_platinum, platinum}},
        });

    return miral::decoration::DecorationBuilder::build(
               decoration->decoration_surface(),
               decoration->window_surface(),
               [decoration](auto event)
               {
                   decoration->handle_input_event(event);
               },
               [decoration](auto... args)
               {
                   decoration->set_scale(args...);
               },
               [decoration](auto... args)
               {
                   decoration->attrib_changed(args...);
               },
               [decoration](auto... args)
               {
                   decoration->window_resized_to(args...);
               },
               [decoration](auto... args)
               {
                   decoration->window_renamed(args...);
               })
        .done();
}

void UserDecorationManagerExample::set_scale(float new_scale)
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

void UserDecorationManagerExample::override_decoration(
    std::shared_ptr<ms::Surface> const& surface, std::unique_ptr<msd::Decoration> decoration)
{
    decorations[surface.get()] = std::move(decoration);
}

std::size_t UserDecorationManagerExample::num_decorations() const
{
    return decorations.size();
}
