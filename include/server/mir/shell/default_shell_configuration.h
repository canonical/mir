/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_SHELL_DEFAULT_SHELL_CONFIGURATION_H_
#define MIR_SHELL_DEFAULT_SHELL_CONFIGURATION_H_

#include "mir/shell/shell_configuration.h"
#include "mir/cached_ptr.h"

namespace mir
{
namespace graphics
{
class ViewableArea;
}
namespace shell
{
class InputTargetListener;
class SurfaceFactory;
class PlacementStrategy;

class DefaultShellConfiguration : public ShellConfiguration
{
public:
    DefaultShellConfiguration(std::shared_ptr<graphics::ViewableArea> const& view_area,
                              std::shared_ptr<InputTargetListener> const& target_listener,
                              std::shared_ptr<SurfaceFactory> const& surface_factory);
    virtual ~DefaultShellConfiguration() noexcept(true) = default;

    std::shared_ptr<SurfaceFactory> the_surface_factory();
    std::shared_ptr<SessionContainer> the_session_container();
    std::shared_ptr<FocusSequence> the_focus_sequence();
    std::shared_ptr<FocusSetter> the_focus_setter();
    std::shared_ptr<InputTargetListener> the_input_target_listener();

    std::shared_ptr<PlacementStrategy> the_placement_strategy();

protected:
    DefaultShellConfiguration(DefaultShellConfiguration const&) = delete;
    DefaultShellConfiguration& operator=(DefaultShellConfiguration const&) = delete;

private:
    std::shared_ptr<graphics::ViewableArea> const view_area;
    std::shared_ptr<InputTargetListener> const input_target_listener;
    std::shared_ptr<SurfaceFactory> const underlying_surface_factory;

    CachedPtr<SurfaceFactory> surface_factory;
    CachedPtr<SessionContainer> session_container;
    CachedPtr<FocusSequence> focus_sequence;
    CachedPtr<FocusSetter> focus_setter;
    CachedPtr<InputTargetListener> input_listener;
    CachedPtr<PlacementStrategy> placement_strategy;
};

}
} // namespace mir

#endif // MIR_SHELL_DEFAULT_SHELL_CONFIGURATION_H_
