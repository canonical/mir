/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_DISPATCH_READABLE_FD_H_
#define MIR_DISPATCH_READABLE_FD_H_

#include "mir/dispatch/dispatchable.h"
#include "mir/fd.h"

#include <functional>

namespace mir
{
namespace dispatch
{

class ReadableFd : public Dispatchable
{
public:
    ReadableFd(Fd fd, std::function<void()> const& on_readable);
    Fd watch_fd() const override;

    bool dispatch(FdEvents events) override;
    FdEvents relevant_events() const override;
private:
    mir::Fd fd;
    std::function<void()> readable;
};
}
}

#endif // MIR_DISPATCH_READABLE_FD_H_
