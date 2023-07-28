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
#include "input-method-unstable-v1_wrapper.cpp" // TODO: Super temporary. Can't figure out why link is broken.
#include <iostream>


namespace mf = mir::frontend;
namespace ms = mir::scene;

/// Handles activation and deactivation of the InputMethodContextV1
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
        context = std::make_unique<InputMethodContextV1>(*static_cast<wayland::InputMethodV1*>(this));
        send_activate_event(context->resource);
    }

    void deactivated()
    {
        if (context)
        {
            auto resource = context->resource;
            context = nullptr;
            send_deactivate_event(resource);
        }
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

    class InputMethodContextV1 : public wayland::InputMethodContextV1
    {
    public:
        InputMethodContextV1(const wayland::InputMethodV1& method)
            : wayland::InputMethodContextV1(method)
        {
        }

        ~InputMethodContextV1() override
        {
            
        }

    private:
        void commit_string(uint32_t /*serial*/, const std::string &/*text*/) override
        {

        }

        void preedit_string(uint32_t /*serial*/, const std::string &/*text*/, const std::string &/*commit*/) override
        {

        }

        void preedit_styling(uint32_t /*index*/, uint32_t /*length*/, uint32_t /*style*/) override
        {

        }

        void preedit_cursor(int32_t /*index*/) override
        {

        }

        void delete_surrounding_text(int32_t /*index*/, uint32_t /*length*/) override
        {

        }

        void cursor_position(int32_t /*index*/, int32_t /*anchor*/) override
        {

        }

        void modifiers_map(struct wl_array */*map*/) override
        {
            std::cout << "Hello world" << std::endl;
            std::cout.flush();
        }

        void keysym(uint32_t /*serial*/, uint32_t /*time*/, uint32_t /*sym*/, uint32_t /*state*/, uint32_t /*modifiers*/) override
        {

        }

        void grab_keyboard(struct wl_resource */*keyboard*/) override
        {

        }

        void key(uint32_t /*serial*/, uint32_t /*time*/, uint32_t /*key*/, uint32_t /*state*/) override
        {

        }

        void modifiers(uint32_t /*serial*/, uint32_t /*mods_depressed*/, uint32_t /*mods_latched*/, uint32_t /*mods_locked*/,
            uint32_t /*group*/) override
        {

        }

        void language(uint32_t /*serial*/, const std::string &/*language*/) override
        {

        }

        void text_direction(uint32_t /*serial*/, uint32_t /*direction*/) override
        {

        }
    };

    mf::InputMethodV1* method;
    std::shared_ptr<StateObserver> const state_observer;
    std::unique_ptr<InputMethodContextV1> context = nullptr;
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
