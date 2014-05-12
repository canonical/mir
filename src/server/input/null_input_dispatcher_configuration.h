/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_NULL_INPUT_DISPATCHER_CONFIGURATION_H_
#define MIR_INPUT_NULL_INPUT_DISPATCHER_CONFIGURATION_H_

#include "mir/input/input_dispatcher_configuration.h"

namespace mir
{
namespace input
{

class NullInputDispatcherConfiguration : public InputDispatcherConfiguration
{
public:
    NullInputDispatcherConfiguration() = default;
    std::shared_ptr<shell::InputTargeter> the_input_targeter() override;
    std::shared_ptr<InputDispatcher> the_input_dispatcher() override;
    bool is_key_repeat_enabled() const override;
protected:
    NullInputDispatcherConfiguration(const NullInputDispatcherConfiguration&) = delete;
    NullInputDispatcherConfiguration& operator=(const NullInputDispatcherConfiguration&) = delete;
};

}
}

#endif

