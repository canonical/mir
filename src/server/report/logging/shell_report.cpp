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

#include "shell_report.h"
#include "mir/logging/logger.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/event_printer.h"

#include <boost/lexical_cast.hpp>

using mir::scene::Session;
using mir::scene::Surface;

namespace
{
char const* const component = "shell::Shell";

void log_basics(std::ostream& out, Session const& session, Surface const& surface, char const* action)
{
    using namespace mir;

    out << '\"' << session.name() << "\" " << action
        << ' ' << surface.type() << " surface[" << &surface << "] \"" << surface.name()
        << "\" @" << surface.input_bounds();
}
}

namespace mrl = mir::logging;

mrl::ShellReport::ShellReport(std::shared_ptr<mir::logging::Logger> const& log) :
    log(log)
{
}

void mrl::ShellReport::opened_session(Session const& session)
{
    log->log(Severity::informational, "session \"" + session.name() + "\" opened", component);
}

void mrl::ShellReport::closing_session(Session const& session)
{
    log->log(Severity::informational, "session \"" + session.name() + "\" closing", component);
}

void mrl::ShellReport::created_surface(
    Session const& session,
    scene::Surface const& surface)
{
    std::ostringstream out;

    log_basics(out, session, surface, "created");

    if (auto const parent = surface.parent())
        out << ", parent=\"" << parent->name() << "\"";

    log->log(Severity::informational, out.str(), component);
}

void mrl::ShellReport::update_surface(
    Session const& session,
    Surface const& surface,
    shell::SurfaceSpecification const& /*modifications*/)
{
    std::ostringstream out;
    log_basics(out, session, surface, "update");
    log->log(Severity::informational, out.str(), component);
}

void mrl::ShellReport::update_surface(
    Session const& session,
    Surface const& surface,
    MirWindowAttrib /*attrib*/, int /*value*/)
{
    std::ostringstream out;
    log_basics(out, session, surface, "update");
    log->log(Severity::informational, out.str(), component);
}

void mrl::ShellReport::destroying_surface(
    Session const& session,
    scene::Surface const& surface)
{
    std::ostringstream out;
    log_basics(out, session, surface, "destroying");
    log->log(Severity::informational, out.str(), component);
}

void mrl::ShellReport::started_prompt_session(
    scene::PromptSession const& /*prompt_session*/,
    Session const& /*session*/)
{
}

void mrl::ShellReport::added_prompt_provider(
    scene::PromptSession const& /*prompt_session*/,
    Session const& /*session*/)
{
}

void mrl::ShellReport::stopping_prompt_session(
    scene::PromptSession const& /*prompt_session*/)
{
}

void mrl::ShellReport::adding_display(geometry::Rectangle const& area)
{
    log->log(Severity::informational, "Adding display area: " + boost::lexical_cast<std::string>(area), component);
}

void mrl::ShellReport::removing_display(geometry::Rectangle const& area)
{
    log->log(Severity::informational, "Removing display area: " + boost::lexical_cast<std::string>(area), component);
}

void mrl::ShellReport::input_focus_set_to(
    Session const* focus_session,
    Surface const* focus_surface)
{
    std::ostringstream out;

    if (focus_session && focus_session)
    {
        log_basics(out, *focus_session, *focus_surface, "input focus");
    }
    else
    {
        auto const session_name = focus_session ? focus_session->name() : "(none)";
        auto const surface_name = focus_surface ? focus_surface->name() : "(none)";

        out << '\"' << session_name << "\" input focus surface[" << focus_surface << "] \"" << surface_name;
    }

    log->log(Severity::informational, out.str(), component);
}

void mrl::ShellReport::surfaces_raised(shell::SurfaceSet const& surfaces)
{
    log->log(Severity::informational, "Raising " + boost::lexical_cast<std::string>(surfaces.size()) + " surfaces", component);
}
