/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_SHELL_SURFACE_STACK_WRAPPER_H
#define MIR_SHELL_SURFACE_STACK_WRAPPER_H

#include "mir/shell/surface_stack.h"

namespace mir
{
namespace shell
{
class SurfaceStackWrapper : public SurfaceStack
{
public:
    explicit SurfaceStackWrapper(std::shared_ptr<SurfaceStack> const& wrapped);

    void add_surface(
        std::shared_ptr<scene::Surface> const&,
        input::InputReceptionMode new_mode) override;

    void add_surface_below_top(
        std::shared_ptr<scene::Surface> const&,
        input::InputReceptionMode new_mode) override;

    void raise(std::weak_ptr<scene::Surface> const& surface) override;

    void raise(scene::SurfaceSet const& surfaces) override;

    void remove_surface(std::weak_ptr<scene::Surface> const& surface) override;

    auto surface_at(geometry::Point) const -> std::shared_ptr<scene::Surface> override;

    void swap_z_order(scene::SurfaceSet const& first, scene::SurfaceSet const& second) override;

    void send_to_back(scene::SurfaceSet const& surfaces) override;

protected:
    std::shared_ptr<SurfaceStack> const wrapped;
};
}
}

#endif //MIR_SHELL_SURFACE_STACK_WRAPPER_H
