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

namespace mir
{
namespace frontend
{
class InputTriggerActionManagerV1 : public wayland::InputTriggerActionManagerV1::Global
{
public:
    InputTriggerActionManagerV1(wl_display* display, std::shared_ptr<InputTriggerData> const& itd) :
        Global{display, Version<1>{}},
        itd{itd}
    {
    }

private:
    class Instance : public wayland::InputTriggerActionManagerV1
    {
        std::shared_ptr<InputTriggerData> const itd;

        void get_input_trigger_action(std::string const& token, struct wl_resource* id) override
        {
            // TODO validate token
            auto const action = new InputTriggerActionV1(id);
            itd->registered_actions.lock()->insert({token, wayland::make_weak(action)});

            action->add_destroy_listener([token, itd = itd] { itd->registered_actions.lock()->erase(token); });
        }

    public:
        Instance(wl_resource* new_ext_input_trigger_action_manager_v1, std::shared_ptr<InputTriggerData> const& itd) :
            InputTriggerActionManagerV1{new_ext_input_trigger_action_manager_v1, Version<1>{}},
            itd{itd}
        {
        }
    };

    void bind(wl_resource* new_ext_input_trigger_action_manager_v1) override
    {
        new Instance{new_ext_input_trigger_action_manager_v1, itd};
    }

    std::shared_ptr<InputTriggerData> const itd;
};

auto create_input_trigger_action_manager_v1(wl_display* display, std::shared_ptr<InputTriggerData> const& itd)
    -> std::shared_ptr<wayland::InputTriggerActionManagerV1::Global>
{
    return std::make_shared<InputTriggerActionManagerV1>(display, itd);
}
}
}
