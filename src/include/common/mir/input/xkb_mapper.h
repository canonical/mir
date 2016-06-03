/*
 * Copyright © 2013-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Robert Carr <robert.carr@canonical.com>
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_RECEIVER_XKB_MAPPER_H_
#define MIR_INPUT_RECEIVER_XKB_MAPPER_H_

#include "mir/input/key_mapper.h"
#include "mir/optional_value.h"

#include <mutex>
#include <unordered_map>

namespace mir
{
namespace input
{

using XKBContextPtr = std::unique_ptr<xkb_context, void(*)(xkb_context*)>;
XKBContextPtr make_unique_context();

using XKBKeymapPtr = std::unique_ptr<xkb_keymap, void(*)(xkb_keymap*)>;
XKBKeymapPtr make_unique_keymap(xkb_context* context, Keymap const& keymap);
XKBKeymapPtr make_unique_keymap(xkb_context* context, char const* buffer, size_t size);

using XKBStatePtr = std::unique_ptr<xkb_state, void(*)(xkb_state*)>;
XKBStatePtr make_unique_state(xkb_keymap* keymap);

namespace receiver
{

class XKBMapper : public KeyMapper
{
public:
    XKBMapper();

    void set_key_state(MirInputDeviceId id, std::vector<uint32_t> const& key_state) override;
    void set_keymap(MirInputDeviceId id, Keymap const& map) override;
    void set_keymap(MirInputDeviceId id, char const* buffer, size_t len) override;
    void set_keymap(Keymap const& map) override;
    void set_keymap(char const* buffer, size_t len) override;
    void reset_keymap(MirInputDeviceId id) override;
    void reset_keymap() override;
    void map_event(MirEvent& event) override;


protected:
    XKBMapper(XKBMapper const&) = delete;
    XKBMapper& operator=(XKBMapper const&) = delete;

private:
    void set_keymap(MirInputDeviceId id, XKBKeymapPtr map);
    void set_keymap(XKBKeymapPtr map);
    void update_modifier();

    std::mutex guard;

    struct XkbMappingState
    {
        explicit XkbMappingState(std::shared_ptr<xkb_keymap> const& keymap);
        void set_key_state(std::vector<uint32_t> const& key_state);
        bool update_and_map(MirEvent& event);
        xkb_keysym_t update_state(uint32_t scan_code, MirKeyboardAction direction);
        std::shared_ptr<xkb_keymap> const keymap;
        XKBStatePtr state;
        MirInputEventModifiers modifier_state{0};
        // TODO optional compose key state..
    };

    XkbMappingState* get_keymapping_state(MirInputDeviceId id);

    XKBContextPtr context;
    std::shared_ptr<xkb_keymap> default_keymap;

    mir::optional_value<MirInputEventModifiers> modifier_state;
    std::unordered_map<MirInputDeviceId, XkbMappingState> device_mapping;
};
}
}
}

#endif // MIR_INPUT_RECEIVER_XKB_MAPPER_H_
