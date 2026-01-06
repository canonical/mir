/*
 * Copyright © Canonical Ltd.
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
 */
#ifndef MIR_FD_H_
#define MIR_FD_H_

#include <memory>

namespace mir
{
/**
 * RAII wrapper for file descriptors.
 *
 * Supports three ownership models:
 * 1. Owned: Fd takes ownership and will close the FD when all references are destroyed
 * 2. Shared: Multiple Fd instances share ownership via reference counting
 * 3. Borrowed: Fd does not own the FD and will not close it
 */
class Fd
{
public:
    /**
     * Transfer ownership of a file descriptor.
     * The Fd will close the file descriptor when all references are destroyed.
     * \param fd The file descriptor to take ownership of
     */
    explicit Fd(int fd);

    /**
     * Create a non-owning reference to a file descriptor.
     * The Fd will NOT close the file descriptor - the caller retains ownership.
     * Use this when you need to pass a borrowed FD that is owned elsewhere.
     * \param fd The file descriptor to borrow
     * \return A non-owning Fd instance
     */
    static auto borrow(int fd) -> Fd;

    static int const invalid{-1};
    Fd(); //Initializes fd to the mir::Fd::invalid;
    Fd(Fd&&);
    Fd(Fd const&) = default;
    Fd& operator=(Fd);

    //bit of a convenient kludge. take care not to close or otherwise destroy the FD.
    operator int() const;

    /// Prevent accidental calling of ::close()
    friend auto close(Fd const& fd) -> int = delete;
private:
    // Private constructor for borrowed FDs
    enum class BorrowTag { Borrow };
    Fd(int fd, BorrowTag);

    std::shared_ptr<int> fd;
};
} // namespace mir

#endif // MIR_FD_H_
