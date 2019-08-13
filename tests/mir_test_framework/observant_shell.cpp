/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir_test_framework/observant_shell.h"

#include "mir/scene/session.h"
#include "mir/scene/surface.h"

namespace msh = mir::shell;
namespace ms = mir::scene;
namespace msc = mir::scene;
namespace mf = mir::frontend;
namespace mtf = mir_test_framework;

mtf::ObservantShell::ObservantShell(
    std::shared_ptr<msh::Shell> const& wrapped,
    std::shared_ptr<msc::SurfaceObserver> const& surface_observer) :
    msh::ShellWrapper(wrapped),
    surface_observer(surface_observer)
{
}

auto mtf::ObservantShell::create_surface(
    std::shared_ptr<msc::Session> const& session,
    msc::SurfaceCreationParameters const& params,
    std::shared_ptr<mf::EventSink> const& sink) -> std::shared_ptr<ms::Surface>
{
    auto window = msh::ShellWrapper::create_surface(session, params, sink);
    window->add_observer(surface_observer);
    return window;
}
