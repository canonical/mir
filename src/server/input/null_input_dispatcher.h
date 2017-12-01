/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_NULL_INPUT_DISPATCHER_H_
#define MIR_INPUT_NULL_INPUT_DISPATCHER_H_

#include "mir/input/input_dispatcher.h"

namespace mir
{
namespace input
{

class NullInputDispatcher : public mir::input::InputDispatcher
{
public:
    bool dispatch(std::shared_ptr<MirEvent const> const& event) override;

    void start() override;
    void stop() override;
};

}
}

#endif
