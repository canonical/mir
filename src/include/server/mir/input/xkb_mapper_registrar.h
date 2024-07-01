/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_INPUT_RECEIVER_XKB_MAPPER_H_
#define MIR_INPUT_RECEIVER_XKB_MAPPER_H_

#include "mir/input/led_observer_registrar.h"
#include "mir/input/key_mapper.h"
#include "mir/input/keymap.h"
#include "mir/input/keyboard_leds.h"
#include "mir/optional_value.h"
#include "mir/events/xkb_modifiers.h"
#include "mir/observer_multiplexer.h"

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace mir
{
namespace input
{

using XKBContextPtr = std::unique_ptr<xkb_context, void(*)(xkb_context*)>;
XKBContextPtr make_unique_context();

using XKBStatePtr = std::unique_ptr<xkb_state, void(*)(xkb_state*)>;
using XKBComposeTablePtr = std::unique_ptr<xkb_compose_table, void(*)(xkb_compose_table*)>;
using XKBComposeStatePtr = std::unique_ptr<xkb_compose_state, void(*)(xkb_compose_state*)>;

namespace receiver
{

class XKBMapperRegistrar : public KeyMapper, public LedObserverRegistrar
{
public:
    explicit XKBMapperRegistrar(Executor&);

    void set_key_state(MirInputDeviceId id, std::vector<uint32_t> const& key_state) override;
    void set_keymap_for_device(MirInputDeviceId id, std::shared_ptr<Keymap> map) override;
    void set_keymap_for_all_devices(std::shared_ptr<Keymap> map) override;
    void clear_keymap_for_device(MirInputDeviceId id) override;
    void clear_all_keymaps() override;
    void map_event(MirEvent& event) override;
    MirInputEventModifiers modifiers() const override;
    MirInputEventModifiers device_modifiers(MirInputDeviceId di) const override;
    auto xkb_modifiers() const -> MirXkbModifiers override;
    void register_interest(std::weak_ptr<LedObserver> const& observer, MirInputDeviceId id) override;
    void unregister_interest(LedObserver const& observer, MirInputDeviceId id) override;

protected:
    XKBMapperRegistrar(XKBMapperRegistrar const&) = delete;
    XKBMapperRegistrar& operator=(XKBMapperRegistrar const&) = delete;

private:
    void set_keymap(MirInputDeviceId id, std::shared_ptr<Keymap> new_keymap);
    void set_keymap(std::shared_ptr<Keymap> new_keymap);
    void update_modifier();

    std::mutex mutable guard;

    struct ComposeState
    {
        ComposeState(XKBComposeTablePtr const& table);
        xkb_keysym_t update_state(xkb_keysym_t mapped_key, MirKeyboardAction action, std::string& text);
    private:
        XKBComposeStatePtr state;
        std::unordered_set<xkb_keysym_t> consumed_keys;
        mir::optional_value<std::tuple<xkb_keysym_t,xkb_keysym_t>> last_composed_key;
    };

    struct XkbMappingState
    {
        class XKBMappingStateRegistrar : public ObserverMultiplexer<LedObserver>
        {
        public:
            explicit XKBMappingStateRegistrar(Executor&);
            void leds_set(KeyboardLeds leds) override;
        };

        explicit XkbMappingState(
            std::shared_ptr<Keymap> keymap,
            std::shared_ptr<xkb_keymap> compiled_keymap,
            Executor& executor);
        void set_key_state(std::vector<uint32_t> const& key_state);

        bool update_and_map(MirEvent& event, ComposeState* compose_state);
        MirInputEventModifiers modifiers() const;
        auto xkb_modifiers() const -> MirXkbModifiers;
        KeyboardLeds get_leds() const;
        XKBMappingStateRegistrar& get_registrar();
    private:
        /// Returns a pair containing the keysym for the given scancode and if any XKB modifiers have been changed
        auto update_state(
            uint32_t scan_code,
            MirKeyboardAction direction,
            ComposeState* compose_state,
            std::string& text) -> std::pair<xkb_keysym_t, bool>;
        void press_modifier(MirInputEventModifiers mod);
        void release_modifier(MirInputEventModifiers mod);

        std::shared_ptr<Keymap> const keymap;
        std::shared_ptr<xkb_keymap> const compiled_keymap;
        XKBStatePtr state;
        MirInputEventModifiers modifier_state{0};
        xkb_led_index_t num_led;
        xkb_led_index_t caps_led;
        xkb_led_index_t scroll_led;
        XKBMappingStateRegistrar registrar;
    };

    XkbMappingState* get_keymapping_state(MirInputDeviceId id);
    ComposeState* get_compose_state(MirInputDeviceId id);

    Executor& executor;
    XKBContextPtr context;
    std::shared_ptr<Keymap> default_keymap;
    std::shared_ptr<xkb_keymap> default_compiled_keymap;
    XKBComposeTablePtr compose_table;
    MirXkbModifiers xkb_modifiers_;
    std::optional<MirInputDeviceId> last_device_id;

    mir::optional_value<MirInputEventModifiers> modifier_state;
    std::unordered_map<MirInputDeviceId, std::unique_ptr<XkbMappingState>> device_mapping;
    std::unordered_map<MirInputDeviceId, std::unique_ptr<ComposeState>> device_composing;
};
}
}
}

#endif // MIR_INPUT_RECEIVER_XKB_MAPPER_H_
