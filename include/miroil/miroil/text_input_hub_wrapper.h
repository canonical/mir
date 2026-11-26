/*
 * Copyright (C) Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MIROIL_TEXT_INPUT_HUB_WRAPPER_H_
#define MIROIL_TEXT_INPUT_HUB_WRAPPER_H_

#include <cstddef>
#include <cstdint>
#include <mir/int_wrapper.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mir
{
namespace scene
{
namespace detail { struct TextInputStateSerialTag; }
class TextInputHub;
class TextInputChangeHandler;
}
}

namespace miroil
{
using TextInputStateSerial = mir::IntWrapper<mir::scene::detail::TextInputStateSerialTag>;
class WrappedTextInputChangeHandler;

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
    none = 0,
    completion = 1 << 0,
    spellcheck = 1 << 1,
    auto_capitalization = 1 << 2,
    lowercase = 1 << 3,
    uppercase = 1 << 4,
    titlecase = 1 << 5,
    hidden_text = 1 << 6,
    sensitive_data = 1 << 7,
    latin = 1 << 8,
    multiline = 1 << 9,
};

struct TextInputChange
{
    TextInputChange() = default;

    TextInputChange(TextInputStateSerial serial) : serial{serial} {}

    struct TextInputPreeditStyle
    {
        uint32_t index;
        uint32_t length;
        uint32_t style;
    };

    struct TextInputKeySym
    {
        uint32_t time;
        uint32_t sym;
        uint32_t state;
        uint32_t modifiers;
    };

    struct CursorPosition
    {
        int index;
        int anchor;
    };

    TextInputStateSerial serial;
    std::optional<std::string> preedit_text;
    std::optional<std::string> preedit_commit;
    std::optional<int> preedit_cursor_begin;
    std::optional<int> preedit_cursor_end;
    std::optional<std::string> commit_text;
    std::optional<int> delete_before;
    std::optional<int> delete_after;

    /// \remark Defined for text input v1 and v2, not v3.
    std::optional<TextInputPreeditStyle> preedit_style;

    /// text-input v1/v2 supports keysym natively, while text-input-v3 requires us to emit a wl_keyboard.key event
    std::optional<TextInputKeySym> keysym;

    /// \remark Defined for text input v1 and v2, not v3.
    std::optional<std::vector<std::byte>> modifier_map;

    /// \remark Defined for text input v1 and v2, not v3.
    std::optional<CursorPosition> cursor_position;

    /// \remark Defined for text input v1 and v2, not v3.
    std::optional<uint32_t> direction;
};

class TextInputChangeHandler
{
public:
    virtual ~TextInputChangeHandler() = default;
    virtual void text_changed(TextInputChange const& change) = 0;
};

struct TextInputState
{
    std::optional<std::string> surrounding_text;
    std::optional<int> cursor;
    std::optional<int> anchor;
    std::optional<TextInputChangeCause> change_cause;
    std::optional<TextInputContentHint> content_hint;
    std::optional<TextInputContentPurpose> content_purpose;
};

class TextInputHubWrapper
{
public:
    TextInputHubWrapper(std::shared_ptr<mir::scene::TextInputHub> const& wrapped);
    ~TextInputHubWrapper() = default;

    auto set_handler_state(
        std::shared_ptr<TextInputChangeHandler> const& handler,
        bool new_input_field,
        TextInputState const& state) -> TextInputStateSerial;
    void deactivate_handler(std::shared_ptr<TextInputChangeHandler> const& handler);

    void show_input_panel();
    void hide_input_panel();

private:
    std::shared_ptr<mir::scene::TextInputHub> wrapped;
    std::shared_ptr<WrappedTextInputChangeHandler> wrapped_handler;
};

}
#endif
