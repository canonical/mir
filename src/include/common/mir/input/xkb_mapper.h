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

#include "mir_toolkit/event.h"

#include <xkbcommon/xkbcommon.h>

#include <memory>
#include <mutex>

namespace mir
{
namespace input
{

using XKBContextPtr = std::unique_ptr<xkb_context, void(*)(xkb_context*)>;
XKBContextPtr make_unique_context();

using XKBKeymapPtr = std::unique_ptr<xkb_keymap, void (*)(xkb_keymap*)>;
XKBKeymapPtr make_unique_keymap(xkb_context* context, std::string const& model, std::string const& layout,
                                std::string const& variant, std::string const& options);
XKBKeymapPtr make_unique_keymap(xkb_context* context, char const* buffer, size_t size);

using XKBStatePtr = std::unique_ptr<xkb_state, void(*)(xkb_state*)>;
XKBStatePtr make_unique_state(xkb_keymap* keymap);

namespace receiver
{

class XKBMapper
{
public:
    XKBMapper();
    virtual ~XKBMapper() = default;

    xkb_context* get_context() const;
    void set_keymap(MirInputDeviceId device_id, XKBKeymapPtr names);

    void update_state_and_map_event(MirEvent& ev);

protected:
    XKBMapper(XKBMapper const&) = delete;
    XKBMapper& operator=(XKBMapper const&) = delete;

private:
    std::mutex guard;

    XKBContextPtr context;
    XKBKeymapPtr keymap;
    XKBStatePtr state;
};

}
}
}

#endif // MIR_INPUT_RECEIVER_XKB_MAPPER_H_
