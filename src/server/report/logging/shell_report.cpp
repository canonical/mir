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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "shell_report.h"
#include "mir/logging/logger.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"

#include <boost/lexical_cast.hpp>

namespace
{
char const* const component = "shell::Shell";
}

namespace mrl = mir::logging;

mrl::ShellReport::ShellReport(std::shared_ptr<mir::logging::Logger> const& log) :
    log(log)
{
}

void mrl::ShellReport::opened_session(scene::Session const& session)
{
    log->log(Severity::informational, "session \"" + session.name() + "\" opened", component);
}

void mrl::ShellReport::closing_session(scene::Session const& session)
{
    log->log(Severity::informational, "session \"" + session.name() + "\" closing", component);
}

void mrl::ShellReport::created_surface(
    scene::Session const& session,
    frontend::SurfaceId surface_id)
{
    auto const surface = session.surface(surface_id);
    std::ostringstream out;
    out << "session \"" << session.name() << "\" created surface: \"" << surface->name() << "\" @";
    out << surface->input_bounds();
    log->log(Severity::informational, out.str(), component);
}

void mrl::ShellReport::update_surface(
    scene::Session const& session,
    scene::Surface const& surface,
    shell::SurfaceSpecification const& /*modifications*/)
{
    std::ostringstream out;
    out << "session \"" << session.name() << "\" update surface: \"" << surface.name() << "\" @";
    out << surface.input_bounds();
    log->log(Severity::informational, out.str(), component);
}

void mrl::ShellReport::update_surface(
    scene::Session const& session,
    scene::Surface const& surface,
    MirSurfaceAttrib /*attrib*/, int /*value*/)
{
    std::ostringstream out;
    out << "session \"" << session.name() << "\" update surface: \"" << surface.name() << "\" @";
    out << surface.input_bounds();
    log->log(Severity::informational, out.str(), component);
}

void mrl::ShellReport::destroying_surface(
    scene::Session const& session,
    frontend::SurfaceId surface_id)
{
    log->log(Severity::informational, "session \"" + session.name() + "\" destroying surface: \"" + session.surface(surface_id)->name() + "\"", component);
}

void mrl::ShellReport::started_prompt_session(
    scene::PromptSession const& /*prompt_session*/,
    scene::Session const& /*session*/)
{
}

void mrl::ShellReport::added_prompt_provider(
    scene::PromptSession const& /*prompt_session*/,
    scene::Session const& /*session*/)
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
    scene::Session const* focus_session,
    scene::Surface const* focus_surface)
{
    auto const session = focus_session ? focus_session->name() : "(none)";
    auto const surface = focus_surface ? focus_surface->name() : "(none)";
    log->log(Severity::informational, "Input focus = session: \"" + session + "\" surface: \"" + surface + "\"", component);
}

void mrl::ShellReport::surfaces_raised(shell::SurfaceSet const& surfaces)
{
    log->log(Severity::informational, "Raising " + boost::lexical_cast<std::string>(surfaces.size()) + " surfaces", component);
}
