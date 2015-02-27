/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SHELL_DEFAULT_SHELL_H_
#define MIR_SHELL_DEFAULT_SHELL_H_

#include "mir/shell/abstract_shell.h"

namespace mir
{
namespace scene { class PlacementStrategy; class SurfaceConfigurator; }

namespace shell
{
/** Default shell implementation.
 * To customise derive from this class and override the methods you want to change
 */
class DefaultShell : public AbstractShell
{
public:
    DefaultShell(
        std::shared_ptr<InputTargeter> const& input_targeter,
        std::shared_ptr<scene::SurfaceCoordinator> const& surface_coordinator,
        std::shared_ptr<scene::SessionCoordinator> const& session_coordinator,
        std::shared_ptr<scene::PromptSessionManager> const& prompt_session_manager,
        std::shared_ptr<scene::PlacementStrategy> const& placement_strategy,
        std::shared_ptr<scene::SurfaceConfigurator> const& surface_configurator);

/** @name these come from frontend::Shell
 *  @{ */
    std::shared_ptr<scene::Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) override;

    void close_session(std::shared_ptr<scene::Session> const& session) override;

    void handle_surface_created(std::shared_ptr<scene::Session> const& session) override;

    frontend::SurfaceId create_surface(std::shared_ptr<scene::Session> const& session, scene::SurfaceCreationParameters const& params) override;

    int set_surface_attribute(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        MirSurfaceAttrib attrib,
        int value) override;

    using AbstractShell::set_focus_to;
/** @} */

    void set_focus_to(std::shared_ptr<scene::Session> const& focus_session);

private:
    std::shared_ptr<scene::PlacementStrategy> const placement_strategy;
    std::shared_ptr<scene::SurfaceConfigurator> const surface_configurator;

    void setting_focus_to(std::shared_ptr<scene::Surface> const& surface) override;
};
}
}

#endif /* MIR_SHELL_DEFAULT_SHELL_H_ */
