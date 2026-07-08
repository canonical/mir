/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_SERVER_DECORATION_MANAGER_H
#define MIR_FRONTEND_SERVER_DECORATION_MANAGER_H

#include "server_decoration.h"

#include <memory>

namespace mir
{
class DecorationStrategy;

namespace frontend
{
class SurfacesWithDecorations;

class ServerDecorationManager : public wayland_rs::ServerDecorationManager
{
public:
    ServerDecorationManager(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::ServerDecorationManagerMiddleware> instance,
        uint32_t object_id,
        std::shared_ptr<DecorationStrategy> strategy);

private:
    auto create(
        wayland_rs::Weak<wayland_rs::Surface> const& surface,
        rust::Box<wayland_rs::ServerDecorationMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::ServerDecoration> override;

    std::shared_ptr<SurfacesWithDecorations> const surfaces_with_decorations;
    std::shared_ptr<DecorationStrategy> const decoration_strategy;
};

} // namespace frontend
} // namespace mir

#endif // MIR_FRONTEND_SERVER_DECORATION_MANAGER_H
