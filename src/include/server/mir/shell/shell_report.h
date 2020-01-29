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

#ifndef MIR_SHELL_SHELL_REPORT_H
#define MIR_SHELL_SHELL_REPORT_H

#include "mir/frontend/surface_id.h"
#include "mir_toolkit/common.h"

#include <memory>
#include <set>

namespace mir
{
namespace geometry { struct Rectangle; }
namespace scene { class PromptSession; class Session; class Surface; struct SurfaceCreationParameters; }

namespace shell
{
struct SurfaceSpecification;
/// @cond
using SurfaceSet = std::set<std::weak_ptr<scene::Surface>, std::owner_less<std::weak_ptr<scene::Surface>>>;
/// @endcond

class ShellReport
{
public:

    virtual void opened_session(scene::Session const& session) = 0;

    virtual void closing_session(scene::Session const& session) = 0;

    virtual void created_surface(
        scene::Session const& session,
        scene::Surface const& surface) = 0;

    virtual void update_surface(
        scene::Session const& session,
        scene::Surface const& surface,
        SurfaceSpecification const& modifications) = 0;

    virtual void update_surface(
        scene::Session const& session, scene::Surface const& surface,
        MirWindowAttrib attrib, int value) = 0;

    virtual void destroying_surface(
        scene::Session const& session,
        scene::Surface const& surface) = 0;

    virtual void started_prompt_session(
        scene::PromptSession const& prompt_session,
        scene::Session const& session) = 0;

    virtual void added_prompt_provider(
        scene::PromptSession const& prompt_session,
        scene::Session const& session) = 0;

    virtual void stopping_prompt_session(
        scene::PromptSession const& prompt_session) = 0;

    virtual void adding_display(geometry::Rectangle const& area) = 0;

    virtual void removing_display(geometry::Rectangle const& area) = 0;

    virtual void input_focus_set_to(
        scene::Session const* focus_session,
        scene::Surface const* focus_surface) = 0;

    virtual void surfaces_raised(SurfaceSet const& surfaces) = 0;

    ShellReport() = default;
    virtual ~ShellReport() = default;
    ShellReport(ShellReport const&) = delete;
    ShellReport& operator=(ShellReport const&) = delete;
};
}
}

#endif //MIR_SHELL_SHELL_REPORT_H
