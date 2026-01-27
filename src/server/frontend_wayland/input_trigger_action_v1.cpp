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

#include "input_trigger_action_v1.h"
#include "input_trigger_data.h"

#include <mir/wayland/protocol_error.h>

namespace mir
{
namespace frontend
{

class InputTriggerActionManagerV1 : public wayland::InputTriggerActionManagerV1::Global
{
public:
    InputTriggerActionManagerV1(
        wl_display* display,
        std::shared_ptr<mir::Synchronised<InputTriggerData>> const& itd,
        std::shared_ptr<shell::TokenAuthority> const& ta,
        std::shared_ptr<input::CompositeEventFilter> const& cef) :
        Global{display, Version<1>{}},
        itd{itd},
        ta{ta},
        cef{cef}
    {
    }

private:
    class Instance : public wayland::InputTriggerActionManagerV1
    {
        std::shared_ptr<mir::Synchronised<InputTriggerData>> const itd;
        std::shared_ptr<shell::TokenAuthority> const ta;
        std::shared_ptr<input::CompositeEventFilter> const cef;

        void get_input_trigger_action(std::string const& token, struct wl_resource* id) override
        {
            auto const& revoked_tokens = itd->lock()->revoked_tokens;
            if (revoked_tokens.contains(token))
            {
                auto const action = InputTriggerActionV1::dummy(id);
                action->send_unavailable_event();
                return;
            }

            auto const itd_ = itd->lock();
            auto& action_controls = itd_->action_controls;
            auto& actions = itd_->actions;
            if (auto const it = action_controls.find(token); it != action_controls.end())
            {
                auto const action = wayland::make_weak(new InputTriggerActionV1(ta, cef, itd_->keyboard_state, id));

                auto& [_, action_control] = *it;
                if (action_control)
                    action_control.value().install_action(action);

                actions.insert({token, action});
            }
            else
            {
                // Token is not currently valid, nor is was it previously revoked. It must be an invalid token.
                BOOST_THROW_EXCEPTION(
                    wayland::ProtocolError(
                        resource,
                        Error::invalid_token,
                        "InputTriggerActionManagerV1::get_input_trigger_action: trying to use a token we never "
                        "issued"));
            }
        }

    public:
        Instance(
            wl_resource* new_ext_input_trigger_action_manager_v1,
            std::shared_ptr<mir::Synchronised<InputTriggerData>> const& itd,
            std::shared_ptr<shell::TokenAuthority> const& ta,
            std::shared_ptr<input::CompositeEventFilter> const& cef) :
            InputTriggerActionManagerV1{new_ext_input_trigger_action_manager_v1, Version<1>{}},
            itd{itd},
            ta{ta},
            cef{cef}
        {
        }
    };

    void bind(wl_resource* new_ext_input_trigger_action_manager_v1) override
    {
        new Instance{new_ext_input_trigger_action_manager_v1, itd, ta, cef};
    }

    std::shared_ptr<mir::Synchronised<InputTriggerData>> const itd;
    std::shared_ptr<shell::TokenAuthority> const ta;
    std::shared_ptr<input::CompositeEventFilter> const cef;
};

auto create_input_trigger_action_manager_v1(
    wl_display* display,
    std::shared_ptr<mir::Synchronised<InputTriggerData>> const& itd,
    std::shared_ptr<shell::TokenAuthority> const& ta,
    std::shared_ptr<input::CompositeEventFilter> const& cef)
    -> std::shared_ptr<wayland::InputTriggerActionManagerV1::Global>
{
    return std::make_shared<InputTriggerActionManagerV1>(display, itd, ta, cef);
}
}
}
