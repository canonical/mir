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

#ifndef MIR_SERVER_FRONTEND_DATA_CONTROL_V1_H_
#define MIR_SERVER_FRONTEND_DATA_CONTROL_V1_H_

#include "ext_data_control_v1.h"

#include <memory>

namespace mir
{
namespace scene
{
class Clipboard;
}
namespace frontend
{
auto create_ext_data_control_manager_v1(
    std::shared_ptr<wayland::Client> client,
    rust::Box<wayland::ExtDataControlManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<mir::scene::Clipboard> const& clipboard,
    std::shared_ptr<mir::scene::Clipboard> const& primary_clipboard)
-> std::shared_ptr<wayland::ExtDataControlManagerV1>;
}
}

#endif
