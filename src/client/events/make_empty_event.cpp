/*
 * Copyright © 2016 Canonical Ltd.
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
 * Author: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "make_empty_event.h"
#include "mir/events/event_private.h"

#include <cstring>
#include <cstdlib>

namespace mev = mir::events;
namespace
{
    void delete_event(MirEvent *e)
    {
        // xkbcommon creates the keymap through malloc
        if (e && e->type == mir_event_type_keymap)
            std::free(const_cast<char*>(e->keymap.buffer));
        delete e;
    }
}

mir::EventUPtr mev::make_empty_event()
{
    auto e = new MirEvent;
    std::memset(e, 0, sizeof (MirEvent));
    return mir::EventUPtr(e, delete_event);
}
