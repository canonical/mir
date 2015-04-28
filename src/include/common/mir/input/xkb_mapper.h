/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
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
namespace receiver
{

class XKBMapper
{
public:
    XKBMapper();
    virtual ~XKBMapper() = default;

    void set_rules(xkb_rule_names const& names);

    void update_state_and_map_event(MirEvent& ev);

protected:
    XKBMapper(XKBMapper const&) = delete;
    XKBMapper& operator=(XKBMapper const&) = delete;

private:
    std::mutex guard;
    
    std::shared_ptr<xkb_context> context;
    std::shared_ptr<xkb_keymap> map;
    std::shared_ptr<xkb_state> state;
};

}
}
}

#endif // MIR_INPUT_RECEIVER_XKB_MAPPER_H_
