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

#ifndef MIR_INPUT_METHOD_COMMON_H
#define MIR_INPUT_METHOD_COMMON_H

#include <cstdint>
#include <mir/scene/text_input_hub.h>
#include "text-input-unstable-v3_wrapper.h"

namespace ms = mir::scene;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{
auto mir_to_wayland_content_hint(ms::TextInputContentHint hint) -> uint32_t;
auto mir_to_wayland_content_purpose(ms::TextInputContentPurpose purpose) -> uint32_t;
auto mir_to_wayland_change_cause(ms::TextInputChangeCause cause) -> uint32_t;
}
}

#endif //MIR_INPUT_METHOD_COMMON_H
