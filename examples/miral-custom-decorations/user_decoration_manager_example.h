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

#ifndef MIRAL_CUSTOM_DECORATIONS_USER_DECORATION_MANAGER_EXAMPLE_H
#define MIRAL_CUSTOM_DECORATIONS_USER_DECORATION_MANAGER_EXAMPLE_H

#include <memory>
#include <unordered_map>

namespace mir::graphics { class DisplayConfigurationObserver; }

namespace mir
{
class Executor;

template <typename Observer>
class ObserverRegistrar;

namespace graphics
{
class GraphicBufferAllocator;
}
namespace shell
{
class Shell;
namespace decoration
{
class Decoration;
}
}
namespace input
{
class CursorImages;
}
namespace scene
{
class Surface;
}
}

class DisplayConfigurationListener;

/// Facilitates decorating windows with Mir's built-in server size decorations
class UserDecorationManagerExample
{
public:
    UserDecorationManagerExample(
        mir::ObserverRegistrar<mir::graphics::DisplayConfigurationObserver>& display_configuration_observers,
        std::shared_ptr<mir::graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<mir::Executor> const& executor,
        std::shared_ptr<mir::input::CursorImages> const& cursor_images);

    virtual ~UserDecorationManagerExample();
    void init(std::weak_ptr<mir::shell::Shell> const& shell);
    void decorate(std::shared_ptr<mir::scene::Surface> const& surface);
    void undecorate(std::shared_ptr<mir::scene::Surface> const& surface);
    void undecorate_all();

protected:
    std::unique_ptr<mir::shell::decoration::Decoration> create_decoration(std::shared_ptr<mir::shell::Shell> const locked_shell, std::shared_ptr<mir::scene::Surface> surface);

    // For testing purposes only
    void override_decoration(std::shared_ptr<mir::scene::Surface> const& surface, std::unique_ptr<mir::shell::decoration::Decoration> decoration);
    std::size_t num_decorations() const;

    std::shared_ptr<mir::graphics::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<mir::Executor> const executor;
    std::shared_ptr<mir::input::CursorImages> const cursor_images;
private:
    std::unordered_map<mir::scene::Surface*, std::unique_ptr<mir::shell::decoration::Decoration>> decorations;

    std::shared_ptr<DisplayConfigurationListener> const display_config_monitor;
    std::weak_ptr<mir::shell::Shell> shell;

    std::mutex mutex;
    float scale{1.0f};

    void set_scale(float new_scale);
};

#endif
