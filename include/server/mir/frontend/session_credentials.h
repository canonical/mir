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
 */

#ifndef MIR_FRONTEND_SESSION_CREDENTIALS_ID_H_
#define MIR_FRONTEND_SESSION_CREDENTIALS_ID_H_

#include <sys/types.h>

namespace mir
{
namespace frontend
{
struct SessionCredentials
{
    pid_t pid() const;
    uid_t uid() const;
    gid_t gid() const;

    static SessionCredentials from_socket(int fd);
    static SessionCredentials from_current_process();

private:
    SessionCredentials(pid_t pid, uid_t uid, gid_t gid);

    pid_t const pid_;
    uid_t const uid_;
    gid_t const gid_;

};
}
}

#endif
