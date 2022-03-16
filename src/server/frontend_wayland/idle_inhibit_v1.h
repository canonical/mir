/*
 * Copyright © 2022 Canonical Ltd.
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
 * Authored by: Grayson Guarino <grayson.guarino@canonical.com>
 */

#ifndef MIR_FRONTEND_IDLE_INHIBIT_V1_H
#define MIR_FRONTEND_IDLE_INHIBIT_V1_H

#include "idle-inhibit-unstable-v1_wrapper.h"

#include <memory>

namespace mir
{
namespace frontend
{
auto create_idle_inhibit_manager_v1(wl_display* display)
-> std::shared_ptr<wayland::IdleInhibitManagerV1::Global>;
}
}

#endif // MIR_FRONTEND_IDLE_INHIBIT_V1_H
