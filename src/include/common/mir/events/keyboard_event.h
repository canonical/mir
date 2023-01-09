/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_COMMON_KEYBOARD_EVENT_H_
#define MIR_COMMON_KEYBOARD_EVENT_H_

#include <cstdint>
#include <string>
#include <memory>

#include "mir/events/input_event.h"
#include "mir/events/xkb_modifiers.h"

namespace mir
{
namespace input
{
class Keymap;
}
}

struct MirKeyboardEvent : MirInputEvent
{
    MirKeyboardEvent();
    auto clone() const -> MirKeyboardEvent* override;

    MirKeyboardAction action() const;
    void set_action(MirKeyboardAction action);

    int32_t keysym() const;
    void set_keysym(int32_t keysym);

    int32_t scan_code() const;
    void set_scan_code(int32_t scan_code);

    char const* text() const;
    void set_text(char const* str);

    std::shared_ptr<mir::input::Keymap> keymap() const;
    void set_keymap(std::shared_ptr<mir::input::Keymap> keymap);

    auto xkb_modifiers() const -> std::optional<MirXkbModifiers>;
    void set_xkb_modifiers(std::optional<MirXkbModifiers> mods);

private:
    MirKeyboardAction action_ = {};
    int32_t keysym_ = 0;
    int32_t scan_code_ = 0;
    std::string text_ = {};
    std::shared_ptr<mir::input::Keymap> keymap_;
    std::optional<MirXkbModifiers> xkb_modifiers_;
};

#endif /* MIR_COMMON_KEYBOARD_EVENT_H_ */
