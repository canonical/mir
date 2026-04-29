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

#ifndef MIR_FRONTEND_XDG_DECORATION_UNSTABLE_V1_H
#define MIR_FRONTEND_XDG_DECORATION_UNSTABLE_V1_H

#include "wayland_rs/wayland_rs_cpp/include/xdg_decoration_unstable_v1.h"
#include "client.h"

namespace mir
{
class DecorationStrategy;

namespace frontend
{
class ToplevelsWithDecorations;
class XdgDecorationManagerV1 : public wayland_rs::ZxdgDecorationManagerV1Impl
{
public:
    XdgDecorationManagerV1(std::shared_ptr<wayland_rs::Client> const& client, std::shared_ptr<DecorationStrategy> strategy);
    auto get_toplevel_decoration(wayland_rs::Weak<wayland_rs::XdgToplevelImpl> const& toplevel) -> std::shared_ptr<wayland_rs::ZxdgToplevelDecorationV1Impl> override;

private:
    std::shared_ptr<wayland_rs::Client> client;
    std::shared_ptr<ToplevelsWithDecorations> const toplevels_with_decorations;
    std::shared_ptr<DecorationStrategy> const decoration_strategy;
};
}
} // namespace mir

#endif // MIR_FRONTEND_RELATIVE_POINTER_UNSTABLE_V1_H
