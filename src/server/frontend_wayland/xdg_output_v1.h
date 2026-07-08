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

#include "xdg_output_unstable_v1.h"

#include <memory>

namespace mir
{
namespace frontend
{
class XdgOutputManagerV1 : public wayland_rs::XdgOutputManagerV1
{
public:
    XdgOutputManagerV1(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::XdgOutputManagerV1Middleware> instance,
        uint32_t object_id);

private:
    auto get_xdg_output(
        wayland_rs::Weak<wayland_rs::Output> const& output,
        rust::Box<wayland_rs::XdgOutputV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::XdgOutputV1> override;
};
}
}

#endif // MIR_FRONTEND_XDG_OUTPUT_V1_H
