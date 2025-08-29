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

#ifndef MIR_FRONTEND_MIR_APPLICATION_SWITCHER_V1_H
#define MIR_FRONTEND_MIR_APPLICATION_SWITCHER_V1_H

#include "mir-application-switcher-unstable-v1_wrapper.h"
#include "mir/shell/application_switcher.h"
#include <memory>

struct wl_display;

namespace mir::frontend
{
auto create_mir_application_switcher(wl_display* display, std::shared_ptr<shell::ApplicationSwitcher> const& app_switcher)
    -> std::shared_ptr<wayland::MirApplicationSwitcherV1::Global>;
}

#endif //MIR_FRONTEND_MIR_APPLICATION_SWITCHER_V1_H