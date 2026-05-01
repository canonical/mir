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

#include "input_method_common.h"
#include "text_input_unstable_v3.h"
#include <boost/throw_exception.hpp>

namespace ms = mir::scene;
namespace mw = mir::wayland_rs;

auto mir::frontend::mir_to_wayland_content_hint(ms::TextInputContentHint hint) -> uint32_t
{
#define MIR_TO_WAYLAND_HINT(name) \
    ((hint & ms::TextInputContentHint::name) != ms::TextInputContentHint::none ? mw::ZwpTextInputV3Impl::ContentHint::name : 0)

    return
        MIR_TO_WAYLAND_HINT(completion) |
        MIR_TO_WAYLAND_HINT(spellcheck) |
        MIR_TO_WAYLAND_HINT(auto_capitalization) |
        MIR_TO_WAYLAND_HINT(lowercase) |
        MIR_TO_WAYLAND_HINT(uppercase) |
        MIR_TO_WAYLAND_HINT(titlecase) |
        MIR_TO_WAYLAND_HINT(hidden_text) |
        MIR_TO_WAYLAND_HINT(sensitive_data) |
        MIR_TO_WAYLAND_HINT(latin) |
        MIR_TO_WAYLAND_HINT(multiline);

#undef MIR_TO_WAYLAND_HINT
}

auto mir::frontend::mir_to_wayland_content_purpose(ms::TextInputContentPurpose purpose) -> uint32_t
{
    switch (purpose)
    {
        case ms::TextInputContentPurpose::normal:
            return mw::ZwpTextInputV3Impl::ContentPurpose::normal;

        case ms::TextInputContentPurpose::alpha:
            return mw::ZwpTextInputV3Impl::ContentPurpose::alpha;

        case ms::TextInputContentPurpose::digits:
            return mw::ZwpTextInputV3Impl::ContentPurpose::digits;

        case ms::TextInputContentPurpose::number:
            return mw::ZwpTextInputV3Impl::ContentPurpose::number;

        case ms::TextInputContentPurpose::phone:
            return mw::ZwpTextInputV3Impl::ContentPurpose::phone;

        case ms::TextInputContentPurpose::url:
            return mw::ZwpTextInputV3Impl::ContentPurpose::url;

        case ms::TextInputContentPurpose::email:
            return mw::ZwpTextInputV3Impl::ContentPurpose::email;

        case ms::TextInputContentPurpose::name:
            return mw::ZwpTextInputV3Impl::ContentPurpose::name;

        case ms::TextInputContentPurpose::password:
            return mw::ZwpTextInputV3Impl::ContentPurpose::password;

        case ms::TextInputContentPurpose::pin:
            return mw::ZwpTextInputV3Impl::ContentPurpose::pin;

        case ms::TextInputContentPurpose::date:
            return mw::ZwpTextInputV3Impl::ContentPurpose::date;

        case ms::TextInputContentPurpose::time:
            return mw::ZwpTextInputV3Impl::ContentPurpose::time;

        case ms::TextInputContentPurpose::datetime:
            return mw::ZwpTextInputV3Impl::ContentPurpose::datetime;

        case ms::TextInputContentPurpose::terminal:
            return mw::ZwpTextInputV3Impl::ContentPurpose::terminal;

        default:
            BOOST_THROW_EXCEPTION(std::invalid_argument{
                "Invalid ms::TextInputContentPurpose " + std::to_string(static_cast<int>(purpose))});
    }
}

auto mir::frontend::mir_to_wayland_change_cause(ms::TextInputChangeCause cause) -> uint32_t
{
    switch (cause)
    {
        case ms::TextInputChangeCause::input_method:
            return mw::ZwpTextInputV3Impl::ChangeCause::input_method;

        case ms::TextInputChangeCause::other:
            return mw::ZwpTextInputV3Impl::ChangeCause::other;

        default:
            BOOST_THROW_EXCEPTION(std::invalid_argument{
                "Invalid ms::TextInputChangeCause " + std::to_string(static_cast<int>(cause))});
    }
}
