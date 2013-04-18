/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_FRONTEND_SERVER_H_
#define MIR_FRONTEND_SERVER_H_

#include "mir_protobuf.pb.h"
#include <memory>

namespace mir
{
class EventSink;

namespace frontend
{

class Server : public mir::protobuf::DisplayServer
{
public:
    virtual ~Server() noexcept;
    void set_event_sink(std::weak_ptr<EventSink> const& sink);

protected:
    std::weak_ptr<EventSink> event_sink;
};

} // namespace frontent
} // namespace mir

#endif
