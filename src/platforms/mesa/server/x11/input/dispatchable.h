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
#include "mir/input/input_sink.h"
#include <X11/Xlib.h>

namespace mir
{

namespace input
{
namespace X
{
struct XDispatchable : public dispatch::Dispatchable
{
public:
    XDispatchable(
        std::shared_ptr<::Display> const& conn,
        int raw_fd);

    mir::Fd watch_fd() const override;
    bool dispatch(dispatch::FdEvents events) override;
    dispatch::FdEvents relevant_events() const override;

    void set_input_sink(InputSink *input_sink);
    void unset_input_sink();

private:
    std::shared_ptr<::Display> const x11_connection;
    mir::Fd fd;
    InputSink *sink;
};

}
}
}

#endif // MIR_INPUT_X_DISPATCHABLE_H_
