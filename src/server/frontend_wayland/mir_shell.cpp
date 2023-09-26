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

#include "mir_shell.h"

namespace mf = mir::frontend;

auto mf::create_mir_shell_v1(struct wl_display* display [[maybe_unused]]) -> std::shared_ptr<wayland::MirShellV1::Global>
{
    return std::shared_ptr<wayland::MirShellV1::Global>{};
}
