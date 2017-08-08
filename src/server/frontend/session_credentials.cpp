/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 */

#include "mir/frontend/session_credentials.h"

namespace mf = mir::frontend;

mf::SessionCredentials::SessionCredentials(pid_t pid, uid_t uid, gid_t gid) :
    the_pid{pid},
    the_uid{uid},
    the_gid{gid}
{
}

pid_t mf::SessionCredentials::pid() const
{
    return the_pid;
}

uid_t mf::SessionCredentials::uid() const
{
    return the_uid;
}

gid_t mf::SessionCredentials::gid() const
{
    return the_gid;
}
