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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/shell/raise_surface_policy.h"

namespace mir
{
namespace scene
{
class Surface;
}
namespace test
{
namespace doubles
{

struct StubRaiseSurfacePolicy : shell::RaiseSurfacePolicy
{
    StubRaiseSurfacePolicy() = default;

    bool should_raise_surface(
        std::shared_ptr<scene::Surface> const& /* surface_candidate */,
        uint64_t /* timestamp */) const override
    {
        return false;
    }
};

}
}
}
