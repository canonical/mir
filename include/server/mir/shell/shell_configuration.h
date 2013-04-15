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

#ifndef MIR_SHELL_SHELL_CONFIGURATION_H_
#define MIR_SHELL_SHELL_CONFIGURATION_H_

#include <memory>

namespace mir
{
namespace shell
{
class SurfaceFactory;
class SessionContainer;
class FocusSequence;
class FocusSetter;
class InputTargetListener;

class ShellConfiguration
{
public:
    virtual ~ShellConfiguration() = default;

    virtual std::shared_ptr<SurfaceFactory> the_surface_factory() = 0;
    virtual std::shared_ptr<SessionContainer> the_session_container() = 0;
    virtual std::shared_ptr<FocusSequence> the_focus_sequence() = 0;
    virtual std::shared_ptr<FocusSetter> the_focus_setter() = 0;
    virtual std::shared_ptr<InputTargetListener> the_input_target_listener() = 0;

protected:
    ShellConfiguration() = default;
    ShellConfiguration(ShellConfiguration const&) = delete;
    ShellConfiguration& operator=(ShellConfiguration const&) = delete;
};

}
} // namespace mir

#endif // MIR_SHELL_SHELL_CONFIGURATION_H_
