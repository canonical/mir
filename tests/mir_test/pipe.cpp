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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include <mir/test/pipe.h>

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <system_error>

#include <unistd.h>
#include <fcntl.h>

namespace mt = mir::test;

mt::Pipe::Pipe()
    : Pipe(0)
{
}

mt::Pipe::Pipe(int flags)
{
    int pipefd[2];
    if (pipe2(pipefd, flags))
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(std::system_error(errno,
                                                       std::system_category(),
                                                       "Failed to create pipe")));
    }
    reader = mir::Fd{pipefd[0]};
    writer = mir::Fd{pipefd[1]};
}

mir::Fd mt::Pipe::read_fd() const
{
    return reader;
}

mir::Fd mt::Pipe::write_fd() const
{
    return writer;
}
