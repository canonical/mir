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

namespace mir
{
namespace frontend
{

class InputTriggerActionV1 : public wayland::InputTriggerActionV1
{
public:
    InputTriggerActionV1(wl_resource* id) :
        wayland::InputTriggerActionV1{id, Version<1>{}}
    {
    }
};

class InputTriggerActionManagerV1 : public wayland::InputTriggerActionManagerV1::Global
{
public:
    InputTriggerActionManagerV1(wl_display* display) :
        Global{display, Version<1>{}}
    {
    }

private:
    class Instance : public wayland::InputTriggerActionManagerV1
    {
        void get_input_trigger_action(std::string const& /*token*/, struct wl_resource* id) override
        {
            // TODO validate token
            new InputTriggerActionV1{id};
        }

    public:
        Instance(wl_resource* new_ext_input_trigger_action_manager_v1) :
            InputTriggerActionManagerV1{new_ext_input_trigger_action_manager_v1, Version<1>{}}
        {
        }
    };

    void bind(wl_resource* new_ext_input_trigger_action_manager_v1) override
    {
        new Instance{new_ext_input_trigger_action_manager_v1};
    }
};

auto create_input_trigger_action_manager_v1(wl_display* display)
    -> std::shared_ptr<wayland::InputTriggerActionManagerV1::Global>
{
    return std::make_shared<InputTriggerActionManagerV1>(display);
}
}
}
