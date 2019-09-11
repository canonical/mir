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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_WRAP_SHELL_TO_TRACK_LATEST_SURFACE_H
#define MIR_TEST_WRAP_SHELL_TO_TRACK_LATEST_SURFACE_H

#include "mir/shell/shell_wrapper.h"
#include "mir/scene/session.h"

namespace mir
{
namespace test
{
namespace doubles
{
struct WrapShellToTrackLatestSurface : mir::shell::ShellWrapper
{
    using mir::shell::ShellWrapper::ShellWrapper;

    auto create_surface(
        std::shared_ptr <mir::scene::Session> const& session,
        mir::scene::SurfaceCreationParameters const& params,
        std::shared_ptr<mir::frontend::EventSink> const& sink) -> std::shared_ptr<scene::Surface> override
    {
        auto const surface = mir::shell::ShellWrapper::create_surface(session, params, sink);
        latest_surface = surface;
        return surface;
    }

    std::weak_ptr <mir::scene::Surface> latest_surface;
};
}
}
}

#endif //MIR_TEST_WRAP_SHELL_TO_TRACK_LATEST_SURFACE_H
