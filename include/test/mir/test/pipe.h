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

#ifndef MIR_TEST_PIPE_H_
#define MIR_TEST_PIPE_H_

#include "mir/fd.h"

namespace mir
{
namespace test
{

class Pipe
{
public:
    Pipe();
    Pipe(int flags);
    ~Pipe() = default;

    Fd read_fd() const;
    Fd write_fd() const;

private:
    Pipe(Pipe const&) = delete;
    Pipe& operator=(Pipe const&) = delete;

    Fd reader;
    Fd writer;
};

}
}

#endif /* MIR_TEST_PIPE_H_ */
