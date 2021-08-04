/*
 * Copyright © 2021 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_SCENE_TEXT_INPUT_HUB_H_
#define MIR_SCENE_TEXT_INPUT_HUB_H_

#include "mir/observer_registrar.h"
#include "mir/int_wrapper.h"

#include <memory>
#include <optional>
#include <string>

namespace mir
{
namespace scene
{
namespace detail
{
struct TextInputStateSerialTag;
}

typedef IntWrapper<detail::TextInputStateSerialTag> TextInputStateSerial;

enum class TextInputChangeCause
{
    input_method,
    other,
};

enum class TextInputContentPurpose
{
    normal,
    alpha,
    digits,
    number,
    phone,
    url,
    email,
    name,
    password,
    pin,
    date,
    time,
    datetime,
    terminal,
};

enum class TextInputContentHint : uint32_t
{
    none                = 0,
    completion          = 1 << 0,
    spellcheck          = 1 << 1,
    auto_capitalization = 1 << 2,
    lowercase           = 1 << 3,
    uppercase           = 1 << 4,
    titlecase           = 1 << 5,
    hidden_text         = 1 << 6,
    sensitive_data      = 1 << 7,
    latin               = 1 << 8,
    multiline           = 1 << 9,
};

inline auto operator|(TextInputContentHint a, TextInputContentHint b) -> TextInputContentHint
{
    return static_cast<TextInputContentHint>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline auto operator&(TextInputContentHint a, TextInputContentHint b) -> TextInputContentHint
{
    return static_cast<TextInputContentHint>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/// TODO: document (see text-input-unstable-v3.xml)
struct TextInputState
{
    std::optional<std::string> surrounding_text;
    std::optional<int> cursor;
    std::optional<int> anchor;
    std::optional<TextInputChangeCause> change_cause;
    std::optional<TextInputContentHint> content_hint;
    std::optional<TextInputContentPurpose> content_purpose;
};

/// TODO: document (see text-input-unstable-v3.xml)
struct TextInputChange
{
    TextInputChange(TextInputStateSerial serial)
        : serial{serial}
    {
    }

    TextInputStateSerial serial;
    std::optional<std::string> preedit_text;
    std::optional<int> preedit_cursor_begin;
    std::optional<int> preedit_cursor_end;
    std::optional<std::string> commit_text;
    std::optional<int> delete_before;
    std::optional<int> delete_after;
};

/// Gets notifications about changes in clients state (implemented by input methods)
class TextInputStateObserver
{
public:
    TextInputStateObserver() = default;
    virtual ~TextInputStateObserver() = default;

    virtual void activated(bool new_input_field, TextInputState const& state) = 0;
    virtual void deactivated() = 0;

private:
    TextInputStateObserver(TextInputStateObserver const&) = delete;
    TextInputStateObserver& operator=(TextInputStateObserver const&) = delete;
};

/// Gets notifications of new text entered by the user
class TextInputChangeHandler
{
public:
    TextInputChangeHandler() = default;
    virtual ~TextInputChangeHandler() = default;

    virtual void change(TextInputChange const& change) = 0;

private:
    TextInputChangeHandler(TextInputChangeHandler const&) = delete;
    TextInputChangeHandler& operator=(TextInputChangeHandler const&) = delete;
};

/// Interface allowing for input methods to get state from and send text input to clients
class TextInputHub : public ObserverRegistrar<TextInputStateObserver>
{
public:
    virtual ~TextInputHub() = default;

    /// Activates input method(s), sets the handler for future changes and sets the state of the text input
    virtual auto set_handler_state(
        std::shared_ptr<TextInputChangeHandler> const& handler,
        bool new_input_field,
        TextInputState const& state) -> TextInputStateSerial = 0;

    /// Deactivates input method(s) if the current handler is set, otherwise does nothing.
    /// NOTE: a handler may be kept alive and changes may be sent to the handler after this returns.
    virtual void deactivate_handler(std::shared_ptr<TextInputChangeHandler> const& handler) = 0;

    /// Used by the input method to dispatch entered text to the handler
    virtual void send_change(TextInputChange const& change) = 0;
};
}
}

#endif // MIR_SCENE_TEXT_INPUT_HUB_H_
