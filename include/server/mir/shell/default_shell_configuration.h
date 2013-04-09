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
 * GNU General Public License for more details.
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
class InputFocusSelector;
class SurfaceFactory;

class DefaultShellConfiguration : public ShellConfiguration
{
public:
    DefaultShellConfiguration(std::shared_ptr<graphics::ViewableArea> const& view_area,
                              std::shared_ptr<InputFocusSelector> const& focus_selector,
                              std::shared_ptr<SurfaceFactory> const& surface_factory);
    virtual ~DefaultShellConfiguration() = default;

    std::shared_ptr<SurfaceFactory> the_surface_factory();
    std::shared_ptr<SessionContainer> the_session_container();
    std::shared_ptr<FocusSequence> the_focus_sequence();
    std::shared_ptr<FocusSetter> the_focus_setter();

protected:
    DefaultShellConfiguration(DefaultShellConfiguration const&) = delete;
    DefaultShellConfiguration& operator=(DefaultShellConfiguration const&) = delete;

private:
    std::shared_ptr<graphics::ViewableArea> const view_area;
    std::shared_ptr<InputFocusSelector> const input_focus_selector;
    std::shared_ptr<SurfaceFactory> const underlying_surface_factory;

    CachedPtr<SurfaceFactory> surface_factory;
    CachedPtr<SessionContainer> session_container;
    CachedPtr<FocusSequence> focus_sequence;
    CachedPtr<FocusSetter> focus_setter;
};

}
} // namespace mir

#endif // MIR_SHELL_DEFAULT_SHELL_CONFIGURATION_H_
