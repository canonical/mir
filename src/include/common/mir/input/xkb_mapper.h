/*
 * Copyright © 2013-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_INPUT_RECEIVER_XKB_MAPPER_H_
#define MIR_INPUT_RECEIVER_XKB_MAPPER_H_

#include "mir/input/key_mapper.h"
#include "mir/input/keymap.h"
#include "mir/optional_value.h"
#include "mir/events/xkb_modifiers.h"

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <mutex>
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

class XKBMapper : public KeyMapper
{
public:
    XKBMapper();

    void set_key_state(MirInputDeviceId id, std::vector<uint32_t> const& key_state) override;
    void set_keymap_for_device(MirInputDeviceId id, std::shared_ptr<Keymap> map) override;
    void set_keymap_for_all_devices(std::shared_ptr<Keymap> map) override;
    void clear_keymap_for_device(MirInputDeviceId id) override;
    void clear_all_keymaps() override;
    void map_event(MirEvent& event) override;
    MirInputEventModifiers modifiers() const override;
    MirInputEventModifiers device_modifiers(MirInputDeviceId di) const override;
    auto xkb_modifiers() const -> MirXkbModifiers override;

protected:
    XKBMapper(XKBMapper const&) = delete;
    XKBMapper& operator=(XKBMapper const&) = delete;

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
        explicit XkbMappingState(std::shared_ptr<Keymap> keymap, std::shared_ptr<xkb_keymap> compiled_keymap);
        void set_key_state(std::vector<uint32_t> const& key_state);

        bool update_and_map(MirEvent& event, ComposeState* compose_state);
        MirInputEventModifiers modifiers() const;
        auto xkb_modifiers() const -> MirXkbModifiers;
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
    };

    XkbMappingState* get_keymapping_state(MirInputDeviceId id);
    ComposeState* get_compose_state(MirInputDeviceId id);

    XKBContextPtr context;
    std::shared_ptr<Keymap> default_keymap;
    std::shared_ptr<xkb_keymap> default_compiled_keymap;
    XKBComposeTablePtr compose_table;
    MirXkbModifiers xkb_modifiers_;
    MirInputDeviceId last_device_id;

    mir::optional_value<MirInputEventModifiers> modifier_state;
    std::unordered_map<MirInputDeviceId, std::unique_ptr<XkbMappingState>> device_mapping;
    std::unordered_map<MirInputDeviceId, std::unique_ptr<ComposeState>> device_composing;
};
}
}
}

#endif // MIR_INPUT_RECEIVER_XKB_MAPPER_H_
