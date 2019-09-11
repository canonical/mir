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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_FRAMEWORK_PLACEMENT_APPLYING_SHELL_H_
#define MIR_TEST_FRAMEWORK_PLACEMENT_APPLYING_SHELL_H_

#include "mir/shell/shell_wrapper.h"
#include "mir/geometry/rectangle.h"

#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"

#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <condition_variable>

namespace mir_test_framework
{
using ClientInputRegions = std::map<std::string, std::vector<mir::geometry::Rectangle>>;
using ClientPositions = std::map<std::string, mir::geometry::Rectangle>;

struct PlacementApplyingShell : mir::shell::ShellWrapper
{
    PlacementApplyingShell(
        std::shared_ptr<mir::shell::Shell> wrapped_coordinator,
        ClientInputRegions const& client_input_regions,
        ClientPositions const& client_positions);

    ~PlacementApplyingShell();
    auto create_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::scene::SurfaceCreationParameters const& params,
        std::shared_ptr<mir::scene::SurfaceObserver> const& observer) -> std::shared_ptr<mir::scene::Surface> override;

    void modify_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& surface,
        mir::shell::SurfaceSpecification const& modifications) override;

    bool wait_for_modify_surface(std::chrono::seconds timeout);

    std::weak_ptr <mir::scene::Surface> latest_surface;

private:
    ClientInputRegions const& client_input_regions;
    ClientPositions const& client_positions;
    std::mutex mutex;
    std::condition_variable cv;
    bool modified {false};
};

}

#endif
