/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_DETAIL_DISPLAY_SERVER_H_
#define MIR_FRONTEND_DETAIL_DISPLAY_SERVER_H_

#include <mir/protobuf/display_server.h>

namespace mir
{
namespace frontend
{
namespace detail
{
class DisplayServer : public mir::protobuf::DisplayServer
{
public:
    virtual void client_pid(int pid) = 0;
};
}
}
}

#endif /* MIR_FRONTEND_DETAIL_DISPLAY_SERVER_H_ */
