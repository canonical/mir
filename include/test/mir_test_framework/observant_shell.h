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

#ifndef MIR_TEST_OBSERVANT_SHELL_H_
#define MIR_TEST_OBSERVANT_SHELL_H_

#include "mir/shell/shell_wrapper.h"

namespace mir { namespace scene { class SurfaceObserver; }}

namespace mir_test_framework
{
struct ObservantShell : mir::shell::ShellWrapper
{
    ObservantShell(
        std::shared_ptr<mir::shell::Shell> const& wrapped,
        std::shared_ptr<mir::scene::SurfaceObserver> const& surface_observer);

    auto create_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::scene::SurfaceCreationParameters const& params,
        std::shared_ptr<mir::frontend::EventSink> const& sink) -> std::shared_ptr<mir::scene::Surface> override;

private:
    std::shared_ptr<mir::scene::SurfaceObserver> const surface_observer;
};
}

#endif /* MIR_TEST_OBSERVANT_SHELL_H_ */
