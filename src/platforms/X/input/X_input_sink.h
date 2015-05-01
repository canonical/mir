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

#ifndef MIR_INPUT_X_INPUT_SINK_H_
#define MIR_INPUT_X_INPUT_SINK_H_

#include "mir/input/input_sink.h"

namespace mir
{
namespace input
{
namespace X
{

struct XInputSink : public input::InputSink
{
public:
	XInputSink() = default;
	~XInputSink() = default;
    void handle_input(MirEvent& event) override;
protected:
    XInputSink(XInputSink const&) = delete;
    XInputSink& operator=(XInputSink const&) = delete;
private:
};

}
}
}

#endif // MIR_INPUT_X_INPUT_SINK_H_
