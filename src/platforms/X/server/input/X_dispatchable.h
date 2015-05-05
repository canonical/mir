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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_INPUT_X_DISPATCHABLE_H_
#define MIR_INPUT_X_DISPATCHABLE_H_

#include "mir/dispatch/dispatchable.h"

namespace mir
{
namespace input
{
namespace X
{

struct XDispatchable : public dispatch::Dispatchable
{
public:
	XDispatchable() = default;
	~XDispatchable() = default;

    XDispatchable(XDispatchable const&) = delete;
    XDispatchable& operator=(XDispatchable const&) = delete;

    mir::Fd watch_fd() const override;
    bool dispatch(dispatch::FdEvents events) override;
    dispatch::FdEvents relevant_events() const override;
};

}
}
}

#endif // MIR_INPUT_X_DISPATCHABLE_H_
