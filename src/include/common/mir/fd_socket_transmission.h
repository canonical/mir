/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_FD_SOCKET_TRANSMISSION_H_
#define MIR_FD_SOCKET_TRANSMISSION_H_
#include "mir/fd.h"
#include <vector>

namespace mir
{
void send_fds(mir::Fd const& socket, std::vector<mir::Fd> const& fd);
}
#endif /* MIR_FD_SOCKET_TRANSMISSION_H_ */
