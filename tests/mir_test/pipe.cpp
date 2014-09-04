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

#include <mir_test/pipe.h>

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <system_error>

#include <unistd.h>

namespace mt = mir::test;

mt::Pipe::Pipe()
{
    if (pipe(pipefd))
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(std::system_error(errno,
                                                       std::system_category(),
                                                       "Failed to create pipe")));
    }
}

mt::Pipe::~Pipe()
{
    close(pipefd[0]);
    close(pipefd[1]);
}

int mt::Pipe::read_fd() const
{
    return pipefd[0];
}

int mt::Pipe::write_fd() const
{
    return pipefd[1];
}
