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
#include "mir/wayland/weak.h"
#include "mir/scene/text_input_hub.h"
#include "wl_surface.h"
#include "input_method_common.h"
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
        scene::TextInputStateSerial serial,
        bool new_input_field,
        scene::TextInputState const& state)
    {
        // Create the new context if we have a new field or if we're not yet activated
        if (!is_activated && new_input_field)
        {
            context = std::make_shared<InputMethodContextV1>(
                this,
                text_input_hub);
            cached_state = ms::TextInputState{};
            send_activate_event(context->resource);
            is_activated = true;
        }

        // Notify about the surrounding text changing
        if (cached_state.surrounding_text != state.surrounding_text ||
            cached_state.cursor != state.cursor ||
            cached_state.anchor != state.anchor)
        {
            cached_state.surrounding_text = state.surrounding_text;
            cached_state.cursor = state.cursor;
            cached_state.anchor = state.anchor;
            context->send_surrounding_text_event(
                state.surrounding_text.value_or(""),
                state.cursor.value_or(0),
                state.anchor.value_or(0));
        }

        // Notify about the new content type
        if (cached_state.content_hint != state.content_hint || cached_state.content_purpose != state.content_purpose)
        {
            cached_state.content_hint = state.content_hint;
            cached_state.content_purpose = state.content_purpose;
            context->send_content_type_event(
                mir_to_wayland_content_hint(state.content_hint.value_or(ms::TextInputContentHint::none)),
                mir_to_wayland_content_purpose(state.content_purpose.value_or(ms::TextInputContentPurpose::normal)));
        }

        //context->send_preferred_language_event("en-us");
        context->add_serial(serial);
    }

    void deactivated()
    {
        if (is_activated && context)
        {
            is_activated = false;
            auto resource = context->resource;
            context_on_deathbed = context;
            send_deactivate_event(resource);
            context = nullptr;
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

    /// https://wayland.app/protocols/input-method-unstable-v1
    /// The InputMethodContextV1 is associated with a single TextInput
    /// and will be destroyed when that text input is no longer receiving text.
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
        }

        void add_serial(ms::TextInputStateSerial serial)
        {
            serials.push_back({done_event_count, serial});
            while (serials.size() > max_remembered_serials)
            {
                serials.pop_front();
            }
            done_event_count++;
            send_commit_state_event(done_event_count);
        }

    private:

        void debug_string(const char* x)
        {
            std::cout << x << std::endl;
            std::cout.flush();
        }

        auto find_serial(uint32_t done_count) const -> std::optional<ms::TextInputStateSerial>
        {
            // Loop in reverse order because the serial we're looking for will generally be at the end
            for (auto it = serials.rbegin(); it != serials.rend(); it++)
            {
                std::cout << "Potential Done : " << it->first << std::endl;
                std::cout << "Potential serial : " << it->second << std::endl;
                std::cout.flush();
                if (it->first == done_count)
                {
                    return it->second;
                }
            }
            return std::nullopt;
        }

        void do_commit(uint32_t serial)
        {
            auto const mir_serial = find_serial(serial);
            if (mir_serial)
            {
                pending_change.serial = *mir_serial;
                text_input_hub->text_changed(pending_change);
            }
            else
            {
                log_warning("%s: invalid commit serial %d", interface_name, serial);
            }
            pending_change = ms::TextInputChange{{}};
        }

        void commit_string(uint32_t serial, const std::string &text) override
        {
            std::cout << "Commit String: Serial = " << serial << std::endl;
            std::cout.flush();
            pending_change.commit_text = text;
            do_commit(serial);
        }

        void preedit_string(uint32_t serial, const std::string &text, const std::string &commit) override
        {
            do_commit(serial);
            pending_change.preedit_text = text;
            pending_change.commit_text = commit;
            do_commit(serial);
        }

        void preedit_styling(uint32_t /*index*/, uint32_t /*length*/, uint32_t /*style*/) override
        {
            debug_string("Preedit style");
        }

        void preedit_cursor(int32_t index) override
        {
            pending_change.preedit_cursor_begin = index;
            pending_change.preedit_cursor_end = index;
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
        std::deque<std::pair<uint32_t , ms::TextInputStateSerial>> serials;
        uint32_t done_event_count{0};
    };

    mf::InputMethodV1* method;
    std::shared_ptr<scene::TextInputHub> const text_input_hub;
    std::shared_ptr<StateObserver> const state_observer;
    bool is_activated = false;
    std::shared_ptr<InputMethodContextV1> context = nullptr;
    std::shared_ptr<InputMethodContextV1> context_on_deathbed = nullptr;
    scene::TextInputState cached_state{};
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