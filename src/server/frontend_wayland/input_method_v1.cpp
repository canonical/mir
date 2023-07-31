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
#include <mir/wayland/weak.h>
#include <mir/scene/text_input_hub.h>
#include "wl_surface.h"
#include "input-method-unstable-v1_wrapper.cpp" // TODO: Super temporary. Can't figure out why link is broken.
#include <iostream>
#include <deque>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland;

/// Handles activation and deactivation of the InputMethodContextV1
class mf::InputMethodV1::Instance : wayland::InputMethodV1
{
private:
    class InputMethodContextV1;

public:
    Instance(wl_resource* new_resource, std::shared_ptr<scene::TextInputHub> const text_input_hub, mf::InputMethodV1* method)
        : InputMethodV1{new_resource, Version<1>()},
          method(method), // TODO: Do we need the method here?
          text_input_hub(text_input_hub),
          state_observer{std::make_shared<StateObserver>(this)}
    {
        text_input_hub->register_interest(state_observer, *method->wayland_executor);
    }

    ~Instance()
    {
        text_input_hub->unregister_interest(*state_observer);
    }

    void activated(
        scene::TextInputStateSerial /*serial*/,
        bool /*new_input_field*/,
        scene::TextInputState const& /*state*/)
    {
        if (context)
        {
            deactivated();
        }

        context = std::make_shared<InputMethodContextV1>(
            this,
            text_input_hub);
        context_list.push_back(context);
        send_activate_event(context->resource);


    }

    void deactivated()
    {
        if (context)
        {
            auto resource = context->resource;
            send_deactivate_event(resource);
            context = nullptr;
        }
    }

    /// TODO: Complexity
    /// The spec calls for the item to be destroyed only after deactivation is handled.
    /// As such, we keep a reference to the InputMethodContextV1 hanging around so that
    /// the wayland "destroy" is not called on a resource that doesn't exist.
    /// \param ctx The destroyed context
    void notify_destroyed(const InputMethodContextV1* ctx)
    {
        auto it = std::find_if(context_list.begin(), context_list.end(), [&](const std::shared_ptr<InputMethodContextV1> other_ctx)
        {
            return other_ctx.get() != ctx;
        });

        if (it != context_list.end())
            context_list.erase(it);
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

    /// https://wayland.app/protocols/input-method-unstable-v1
    class InputMethodContextV1 : public wayland::InputMethodContextV1
    {
    public:
        InputMethodContextV1(
            mf::InputMethodV1::Instance* method,
            std::shared_ptr<scene::TextInputHub> const text_input_hub)
            : wayland::InputMethodContextV1(*static_cast<wayland::InputMethodV1*>(method)),
              method(method),
              text_input_hub(text_input_hub)
        {
        }

        ~InputMethodContextV1()
        {
            method->notify_destroyed(this);
        }

    private:

        void debug_string(const char* x)
        {
            std::cout << x << std::endl;
            std::cout.flush();
        }

        void commit(uint32_t /*serial*/)
        {
            text_input_hub->text_changed(pending_change);
        }

        void commit_string(uint32_t /*serial*/, const std::string &/*text*/) override
        {
            debug_string("Commit string");
//            pending_change.commit_text = text;
//            commit(serial);
        }

        void preedit_string(uint32_t /*serial*/, const std::string &/*text*/, const std::string &/*commit*/) override
        {
            debug_string("Preedit string");
        }

        void preedit_styling(uint32_t /*index*/, uint32_t /*length*/, uint32_t /*style*/) override
        {
            debug_string("Preedit style");
        }

        void preedit_cursor(int32_t /*index*/) override
        {
            debug_string("Preedit");
        }

        void delete_surrounding_text(int32_t /*index*/, uint32_t /*length*/) override
        {
            debug_string("Delete");
        }

        void cursor_position(int32_t /*index*/, int32_t /*anchor*/) override
        {
            debug_string("Cursor");
        }

        void modifiers_map(struct wl_array */*map*/) override
        {
            debug_string("Modifiers Map");
        }

        void keysym(uint32_t /*serial*/, uint32_t /*time*/, uint32_t /*sym*/, uint32_t /*state*/, uint32_t /*modifiers*/) override
        {
            debug_string("keysym");
        }

        void grab_keyboard(struct wl_resource */*keyboard*/) override
        {
            debug_string("Keyboard grabbing");
        }

        void key(uint32_t /*serial*/, uint32_t /*time*/, uint32_t /*key*/, uint32_t /*state*/) override
        {
            debug_string("Keydown");
        }

        void modifiers(uint32_t /*serial*/, uint32_t /*mods_depressed*/, uint32_t /*mods_latched*/, uint32_t /*mods_locked*/,
            uint32_t /*group*/) override
        {
            debug_string("Modifiers");
        }

        void language(uint32_t /*serial*/, const std::string &/*language*/) override
        {
            debug_string("Language");
        }

        void text_direction(uint32_t /*serial*/, uint32_t /*direction*/) override
        {
            debug_string("Text direction");
        }

        mf::InputMethodV1::Instance* method = nullptr;
        std::shared_ptr<scene::TextInputHub> const text_input_hub;
        scene::TextInputChange pending_change{{}};
        static size_t constexpr max_remembered_serials{10};
        std::deque<ms::TextInputStateSerial> serials;
    };

    mf::InputMethodV1* method;
    std::shared_ptr<scene::TextInputHub> const text_input_hub;
    std::shared_ptr<StateObserver> const state_observer;
    std::shared_ptr<InputMethodContextV1> context = nullptr;
    std::vector<std::shared_ptr<InputMethodContextV1>> context_list;
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
    new Instance{new_resource, text_input_hub, this};
}

class mf::InputPanelV1::Instance : wayland::InputPanelV1
{
public:
    Instance(wl_resource* new_resource)
        : InputPanelV1{new_resource, Version<1>()}
    {
    }

private:
    class InputPanelSurfaceV1 : wayland::InputPanelSurfaceV1
    {
    public:
        InputPanelSurfaceV1(wl_resource* resource, wl_resource* surface)
            : wayland::InputPanelSurfaceV1(resource, Version<1>()),
              surface(WlSurface::from(surface))
        {
        }

    private:
        void set_toplevel(struct wl_resource* /*output*/, uint32_t /*position*/) override
        {
        }

        void set_overlay_panel() override
        {
        }

        mw::Weak<WlSurface> const surface;
    };

    void get_input_panel_surface(wl_resource* id, wl_resource* surface) override
    {
        surface_instance = std::make_unique<InputPanelSurfaceV1>(id, surface);
    }

    std::unique_ptr<InputPanelSurfaceV1> surface_instance;
};

mf::InputPanelV1::InputPanelV1(wl_display *display)
    : Global(display, Version<1>()),
      display(display)
{}

void mf::InputPanelV1::bind(wl_resource *new_zwp_input_panel_v1)
{
    new Instance{new_zwp_input_panel_v1};
}