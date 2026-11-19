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
#include <miroil/text_input_hub_wrapper.h>
#include <mir/scene/text_input_hub.h>

namespace miroil
{

class WrappedTextInputChangeHandler : public mir::scene::TextInputChangeHandler
{
public:
    WrappedTextInputChangeHandler() = default;

    void text_changed(mir::scene::TextInputChange const& change) override
    {
        if (wrapped)
        {
            miroil::TextInputChange converted_change;
            converted_change.serial = change.serial;
            converted_change.preedit_text = change.preedit_text;
            converted_change.preedit_commit = change.preedit_commit;
            converted_change.preedit_cursor_begin = change.preedit_cursor_begin;
            converted_change.preedit_cursor_end = change.preedit_cursor_end;
            converted_change.commit_text = change.commit_text;
            converted_change.delete_before = change.delete_before;
            converted_change.delete_after = change.delete_after;
            if (change.preedit_style.has_value())
            {
                converted_change.preedit_style = TextInputChange::TextInputPreeditStyle{
                    change.preedit_style->index,
                    change.preedit_style->length,
                    change.preedit_style->style
                };
            }
            if (change.keysym.has_value())
            {
                converted_change.keysym = TextInputChange::TextInputKeySym{
                    change.keysym->time,
                    change.keysym->sym,
                    change.keysym->state,
                    change.keysym->modifiers
                };
            }
            converted_change.modifier_map = change.modifier_map;
            if (change.cursor_position.has_value())
            {
                converted_change.cursor_position = TextInputChange::CursorPosition{
                    change.cursor_position->index,
                    change.cursor_position->anchor
                };
            }
            converted_change.direction = change.direction;

            wrapped->text_changed(converted_change);
        }
    }

    // miroil's TextInputChangeHandler
    std::shared_ptr<miroil::TextInputChangeHandler> wrapped;
};

TextInputHubWrapper::TextInputHubWrapper(std::shared_ptr<mir::scene::TextInputHub> const& wrapped):
  wrapped(wrapped),
  wrapped_handler(std::make_shared<WrappedTextInputChangeHandler>())
{
}

auto TextInputHubWrapper::set_handler_state(std::shared_ptr<TextInputChangeHandler> const& handler, bool new_input_field, TextInputState const& state) -> TextInputStateSerial
{
    mir::scene::TextInputState mir_state;

    wrapped_handler->wrapped = handler;

    if (state.surrounding_text.has_value())
    {
        mir_state.surrounding_text = std::move(state.surrounding_text.value());
    }
    mir_state.cursor = state.cursor;
    mir_state.anchor = state.anchor;
    if (state.change_cause.has_value())
    {
        mir_state.change_cause = static_cast<mir::scene::TextInputChangeCause>(state.change_cause.value());
    }
    if (state.content_hint.has_value())
    {
        mir_state.content_hint = static_cast<mir::scene::TextInputContentHint>(state.content_hint.value());
    }
    if (state.content_purpose.has_value())
    {
        mir_state.content_purpose = static_cast<mir::scene::TextInputContentPurpose>(state.content_purpose.value());
    }

    return TextInputStateSerial{wrapped->set_handler_state(wrapped_handler, new_input_field, mir_state).as_value()};
}

void TextInputHubWrapper::deactivate_handler(std::shared_ptr<TextInputChangeHandler> const& handler)
{
    if (wrapped_handler->wrapped == handler)
        wrapped->deactivate_handler(wrapped_handler);
}

void TextInputHubWrapper::show_input_panel()
{
    wrapped->show_input_panel();
}

void TextInputHubWrapper::hide_input_panel()
{
    wrapped->hide_input_panel();
}

}
