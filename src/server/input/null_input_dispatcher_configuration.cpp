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

#include "null_input_dispatcher_configuration.h"

#include "mir/input/input_dispatcher.h"
#include "mir/shell/input_targeter.h"

namespace mi = mir::input;
namespace msh = mir::shell;

namespace
{

struct NullInputTargeter : public msh::InputTargeter
{
    NullInputTargeter() = default;
    virtual ~NullInputTargeter() noexcept(true) = default;

    void focus_changed(std::shared_ptr<mi::InputChannel const> const&) override
    {
    }

    void focus_cleared() override
    {
    }
};

class NullInputDispatcher : public mi::InputDispatcher
{
    void configuration_changed(nsecs_t /*when*/) override
    {
    }
    void device_reset(int32_t /*device_id*/, nsecs_t /*when*/) override
    {
    }
    void dispatch(MirEvent const& /*event*/) override
    {
    }
    void start() override
    {
    }
    void stop() override
    {
    }
};

}

std::shared_ptr<msh::InputTargeter> mi::NullInputDispatcherConfiguration::the_input_targeter()
{
    return std::make_shared<NullInputTargeter>();
}

std::shared_ptr<mi::InputDispatcher> mi::NullInputDispatcherConfiguration::the_input_dispatcher()
{
    return std::make_shared<NullInputDispatcher>();
}

bool mi::NullInputDispatcherConfiguration::is_key_repeat_enabled() const
{
    return true;
}

