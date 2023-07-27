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

#include "input_method_v1.h"
#include <mir/scene/text_input_hub.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;

class mf::InputMethodV1::Instance : wayland::InputMethodV1
{
public:
    Instance(wl_resource* new_resource, mf::InputMethodV1* method)
        : InputMethodV1{new_resource, Version<1>()},
          method(method),
          state_observer{std::make_shared<StateObserver>(this)}
    {
        method->text_input_hub->register_interest(state_observer, *method->wayland_executor);
    }

    ~Instance()
    {
        method->text_input_hub->unregister_interest(*state_observer);
    }

    void activated(
        scene::TextInputStateSerial /*serial*/,
        bool /*new_input_field*/,
        scene::TextInputState const& /*state*/)
    {
        //input_method->send_activate_event(input_method->resource);
    }

    void deactivated()
    {
        //input_method->send_deactivate_event(input_method->resource);
    }

private:
    struct StateObserver : ms::TextInputStateObserver
    {
        StateObserver(Instance* input_method)
            : input_method{input_method}
        {
        }

        void activated(
            scene::TextInputStateSerial serial,
            bool new_input_field,
            scene::TextInputState const& state) override
        {
            input_method->activated(serial, new_input_field, state);
        }

        void deactivated() override
        {
            input_method->deactivated();
        }

        Instance* const input_method;
    };

    mf::InputMethodV1* method;
    std::shared_ptr<StateObserver> const state_observer;
};

mf::InputMethodV1::InputMethodV1(
    wl_display *display,
    std::shared_ptr<Executor> const wayland_executor,
    std::shared_ptr<scene::TextInputHub> const text_input_hub)
    : Global(display, Version<1>()),
      display(display),
      wayland_executor(wayland_executor),
      text_input_hub(text_input_hub)
{
}

void mf::InputMethodV1::bind(wl_resource *new_resource)
{
    new Instance{new_resource, this};
}
