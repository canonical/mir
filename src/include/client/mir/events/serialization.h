/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_EVENTS_SERIALIZATIN_H_
#define MIR_EVENTS_SERIALIZATIN_H_

#include "mir_toolkit/event.h"
#include "mir/events/event_builders.h"
#include <string>

namespace mir
{
namespace events
{
std::string serialize_event(MirEvent const& event);
EventUPtr deserialize_event(std::string const& raw);
}
}

#endif
