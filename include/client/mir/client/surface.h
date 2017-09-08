/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_SURFACE_H
#define MIRAL_SURFACE_H

#include <mir_toolkit/rs/mir_render_surface.h>

#include <memory>

namespace mir
{
namespace client
{
/// Handle class for MirRenderSurface - provides automatic reference counting.
class Surface
{
public:
    Surface() = default;
    explicit Surface(MirRenderSurface* spec) : self{spec, &mir_render_surface_release} {}

    operator MirRenderSurface*() const { return self.get(); }

    void reset() { self.reset(); }

private:
    std::shared_ptr<MirRenderSurface> self;
};

// Provide a deleted overload to avoid double release "accidents".
void mir_render_surface_release(Surface const& surface) = delete;
}
}

#endif //MIRAL_SURFACE_H
