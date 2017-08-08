/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#ifndef MIR_CLIENT_EVENT_DISTRIBUTOR_H_
#define MIR_CLIENT_EVENT_DISTRIBUTOR_H_

#include "event_sink.h"
#include "event_handler_register.h"

namespace mir
{
namespace client
{

class EventDistributor : public EventHandlerRegister, public EventSink
{
};

} // namespace client
} // namespace mir

#endif // MIR_CLIENT_EVENT_DISTRIBUTOR_H_
