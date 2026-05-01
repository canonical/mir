/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_XDG_OUTPUT_V1_H
#define MIR_FRONTEND_XDG_OUTPUT_V1_H

#include "wayland_rs/wayland_rs_cpp/include/xdg_output_unstable_v1.h"
#include "client.h"
#include <memory>

struct wl_display;

namespace mir
{
namespace frontend
{

class XdgOutputManagerV1 : public wayland_rs::ZxdgOutputManagerV1Impl
{
public:
    XdgOutputManagerV1(std::shared_ptr<wayland_rs::Client> const&);
    auto get_xdg_output(wayland_rs::Weak<wayland_rs::WlOutputImpl> const& output)
        -> std::shared_ptr<wayland_rs::ZxdgOutputV1Impl> override;

private:
    std::shared_ptr<wayland_rs::Client> client;
};
}
}

#endif // MIR_FRONTEND_XDG_OUTPUT_V1_H
