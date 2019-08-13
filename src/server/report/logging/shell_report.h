/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_REPORT_NULL_SHELL_REPORT_H
#define MIR_REPORT_NULL_SHELL_REPORT_H

#include "mir/shell/shell_report.h"

namespace mir
{
namespace logging
{
class Logger;

class ShellReport : public shell::ShellReport
{
public:
    ShellReport(std::shared_ptr<mir::logging::Logger> const& log);

    void opened_session(scene::Session const& session) override;

    void closing_session(scene::Session const& session) override;

    void created_surface(
        scene::Session const& session,
        scene::Surface const& surface) override;

    void update_surface(
        scene::Session const& session,
        scene::Surface const& surface,
        shell::SurfaceSpecification const& modifications) override;

    void update_surface(
        scene::Session const& session,
        scene::Surface const& surface,
        MirWindowAttrib attrib, int value) override;

    void destroying_surface(
        scene::Session const& session,
        scene::Surface const& surface) override;

    void started_prompt_session(
        scene::PromptSession const& prompt_session,
        scene::Session const& session) override;

    void added_prompt_provider(
        scene::PromptSession const& prompt_session,
        scene::Session const& session) override;

    void stopping_prompt_session(
        scene::PromptSession const& prompt_session) override;

    void adding_display(geometry::Rectangle const& area) override;

    void removing_display(geometry::Rectangle const& area) override;

    void input_focus_set_to(
        scene::Session const* focus_session,
        scene::Surface const* focus_surface) override;

    void surfaces_raised(shell::SurfaceSet const& surfaces) override;

private:
    std::shared_ptr<mir::logging::Logger> const log;
};
}
}

#endif //MIR_REPORT_NULL_SHELL_REPORT_H
