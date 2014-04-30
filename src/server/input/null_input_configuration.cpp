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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "null_input_configuration.h"

#include "mir/input/input_dispatcher_configuration.h"
#include "mir/input/input_dispatcher.h"
#include "mir/input/input_channel_factory.h"
#include "mir/scene/input_registrar.h"
#include "mir/shell/input_targeter.h"
#include "mir/input/input_manager.h"

namespace mi = mir::input;
namespace ms = mir::scene;
namespace msh = mir::shell;

namespace
{

struct NullInputRegistrar : public ms::InputRegistrar
{
    NullInputRegistrar() = default;
    virtual ~NullInputRegistrar() noexcept(true) = default;

    void input_channel_opened(std::shared_ptr<mi::InputChannel> const&,
                              std::shared_ptr<mi::Surface> const&,
                              mi::InputReceptionMode /* receives_all_input */) override
    {
    }

    void input_channel_closed(std::shared_ptr<mi::InputChannel> const&) override
    {
    }
};

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

class NullInputChannelFactory : public mi::InputChannelFactory
{
    std::shared_ptr<mi::InputChannel> make_input_channel() override
    {
        return std::shared_ptr<mi::InputChannel>();
    }
};

class NullInputManager : public mi::InputManager
{
    void start() override
    {
    }
    void stop() override
    {
    }
};

class NullInputDispatcher : public mi::InputDispatcher
{
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


class NullInputDispatcherConfiguration : public mi::InputDispatcherConfiguration
{
public:
    NullInputDispatcherConfiguration() = default;
    std::shared_ptr<ms::InputRegistrar> the_input_registrar() override
    {
        return std::make_shared<NullInputRegistrar>();
    }

    std::shared_ptr<msh::InputTargeter> the_input_targeter() override
    {
        return std::make_shared<NullInputTargeter>();
    }

    std::shared_ptr<mi::InputDispatcher> the_input_dispatcher() override
    {
        return std::make_shared<NullInputDispatcher>();
    }

    bool is_key_repeat_enabled() const override
    {
        return true;
    }

    void set_input_targets(std::shared_ptr<mi::InputTargets> const& /*targets*/) override
    {
    }
protected:
    NullInputDispatcherConfiguration(const NullInputDispatcherConfiguration&) = delete;
    NullInputDispatcherConfiguration& operator=(const NullInputDispatcherConfiguration&) = delete;
};

}

std::shared_ptr<mi::InputDispatcherConfiguration> mi::NullInputConfiguration::the_input_dispatcher_configuration()
{
    return std::make_shared<NullInputDispatcherConfiguration>();
}


std::shared_ptr<mi::InputChannelFactory> mi::NullInputConfiguration::the_input_channel_factory()
{
    return std::make_shared<NullInputChannelFactory>();
}


std::shared_ptr<mi::InputManager> mi::NullInputConfiguration::the_input_manager()
{
    return std::make_shared<NullInputManager>();
}

