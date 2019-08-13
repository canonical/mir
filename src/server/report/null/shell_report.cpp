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

namespace mrn = mir::report::null;

void mrn::ShellReport::opened_session(scene::Session const& /*session*/)
{
}

void mrn::ShellReport::closing_session(scene::Session const& /*session*/)
{
}

void mrn::ShellReport::created_surface(
    scene::Session const& /*session*/,
    scene::Surface const& /*surface_id*/)
{
}

void mrn::ShellReport::update_surface(
    scene::Session const& /*session*/,
    scene::Surface const& /*surface*/,
    shell::SurfaceSpecification const& /*modifications*/)
{
}

void mrn::ShellReport::update_surface(
    scene::Session const& /*session*/,
    scene::Surface const& /*surface*/,
    MirWindowAttrib /*attrib*/, int /*value*/)
{
}

void mrn::ShellReport::destroying_surface(
    scene::Session const& /*session*/,
    scene::Surface const& /*surface*/)
{
}

void mrn::ShellReport::started_prompt_session(
    scene::PromptSession const& /*prompt_session*/,
    scene::Session const& /*session*/)
{
}

void mrn::ShellReport::added_prompt_provider(
    scene::PromptSession const& /*prompt_session*/,
    scene::Session const& /*session*/)
{
}

void mrn::ShellReport::stopping_prompt_session(
    scene::PromptSession const& /*prompt_session*/)
{
}

void mrn::ShellReport::adding_display(geometry::Rectangle const& /*area*/)
{
}

void mrn::ShellReport::removing_display(geometry::Rectangle const& /*area*/)
{
}

void mrn::ShellReport::input_focus_set_to(
    scene::Session const* /*focus_session*/,
    scene::Surface const* /*focus_surface*/)
{
}

void mrn::ShellReport::surfaces_raised(shell::SurfaceSet const& /*surfaces*/)
{
}
