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

#ifndef MIR_SHELL_ACCESSIBILITY_MANAGER_H
#define MIR_SHELL_ACCESSIBILITY_MANAGER_H

#include "mir/input/input_event_transformer.h"
#include "mir/synchronised.h"

#include <memory>
#include <optional>
#include <vector>

namespace mir
{
namespace options
{
class Option;
}
namespace graphics
{
class Cursor;
}
namespace shell
{
class KeyboardHelper;
class AccessibilityManager
{
public:
    AccessibilityManager(
        std::shared_ptr<mir::options::Option> const&,
        std::shared_ptr<input::InputEventTransformer> const& event_transformer,
        std::shared_ptr<mir::graphics::Cursor> const& cursor);

    void register_keyboard_helper(std::shared_ptr<shell::KeyboardHelper> const&);

    std::optional<int> repeat_rate() const;
    int repeat_delay() const;

    void repeat_rate(int new_rate);
    void repeat_delay(int new_rate);

    void notify_helpers() const;

    void cursor_scale_changed(float new_scale);

private:

    struct MutableState {
        // 25 rate and 600 delay are the default in Weston and Sway
        int repeat_rate{25};
        int repeat_delay{600};

        std::vector<std::shared_ptr<shell::KeyboardHelper>> keyboard_helpers;

        float cursor_scale{1};
    };

    Synchronised<MutableState> mutable_state;

    bool const enable_key_repeat;
    bool const enable_mouse_keys;

    std::shared_ptr<mir::input::InputEventTransformer> const event_transformer;
    std::shared_ptr<mir::input::InputEventTransformer::Transformer> const transformer;
    std::shared_ptr<graphics::Cursor> const cursor;
};
}
}

#endif
