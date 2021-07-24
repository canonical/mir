/*
 * Copyright © 2021 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_TEXT_INPUT_V3_H
#define MIR_FRONTEND_TEXT_INPUT_V3_H

#include <memory>
struct wl_display;

namespace mir
{
namespace frontend
{
class TextInputManagerV3Global;

auto create_text_input_manager_v3(wl_display* display) -> std::shared_ptr<TextInputManagerV3Global>;
}
}

#endif // MIR_FRONTEND_TEXT_INPUT_V3_H
