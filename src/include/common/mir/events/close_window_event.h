/*
 * Copyright © Canonical Ltd.
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

#ifndef MIR_COMMON_CLOSE_WINDOW_EVENT_H_
#define MIR_COMMON_CLOSE_WINDOW_EVENT_H_

#include "mir/events/event.h"

struct MirCloseWindowEvent : MirEvent
{
    MirCloseWindowEvent();
    auto clone() const -> MirCloseWindowEvent* override;

    int surface_id() const;
    void set_surface_id(int id);

private:
    int surface_id_ = 0;
};

#endif /* MIR_COMMON_CLOSE_WINDOW_EVENT_H_ */
