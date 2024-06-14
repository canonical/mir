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

#include "xdg-decoration-unstable-v1_wrapper.h"

namespace mir
{
namespace frontend
{
auto create_xdg_decoration_unstable_v1(
    wl_display* display)
-> std::shared_ptr<wayland::XdgDecorationManagerV1::Global>;
}
}

#endif  // MIR_FRONTEND_RELATIVE_POINTER_UNSTABLE_V1_H
