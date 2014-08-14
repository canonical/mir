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
#ifndef MIR_FD_H_
#define MIR_FD_H_

#include <memory>

namespace mir
{
//TODO: remove once mir::Fd is used more pervasively.
//      some existing code does not really allow us to transfer or share the ownership
//      of the fd. Constructing using mir::Fd(IntOwnedFd(int)) will help transition the existing
//      code to using the mir::Fd type properly. 
struct IntOwnedFd
{
    int int_owned_fd;
};
class Fd
{
public:
    //transfer ownership of the POD-int to the object. The int no longer needs close()ing,
    //and has the lifetime of the Fd object.
    explicit Fd(int fd);
    explicit Fd(IntOwnedFd);
    static int const invalid{-1};
    Fd(); //Initializes fd to the mir::Fd::invalid;
    Fd(Fd&&);
    Fd(Fd const&) = default;
    Fd& operator=(Fd);

    //bit of a convenient kludge. take care not to close or otherwise destroy the FD.
    operator int() const;

private:
    std::shared_ptr<int> fd;
};
} // namespace mir

#endif // MIR_FD_H_
