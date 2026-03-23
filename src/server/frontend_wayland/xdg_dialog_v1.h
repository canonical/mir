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

#ifndef MIR_FRONTEND_XDG_DIALOG_V1_H
#define MIR_FRONTEND_XDG_DIALOG_V1_H

#include "xdg-dialog-v1_wrapper.h"

struct wl_display;

namespace mir
{
namespace frontend
{
auto create_xdg_dialog_v1(struct wl_display* display) -> std::shared_ptr<wayland::XdgWmDialogV1::Global>;
}
}

#endif // MIR_FRONTEND_XDG_DIALOG_V1_H
