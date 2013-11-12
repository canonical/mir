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

#include "mir/input/null_input_configuration.h"

#include "mir/surfaces/input_registrar.h"
#include "mir/shell/input_targeter.h"
#include "mir/input/input_manager.h"

namespace mi = mir::input;
namespace ms = mir::surfaces;
namespace msh = mir::shell;

namespace
{

struct NullInputRegistrar : public ms::InputRegistrar
{
    NullInputRegistrar() = default;
    virtual ~NullInputRegistrar() noexcept(true) = default;
    
    void input_channel_opened(std::shared_ptr<mi::InputChannel> const&,
                              std::shared_ptr<mi::Surface> const&,
                              mi::InputReceptionMode /* receives_all_input */)
    {
    }

    void input_channel_closed(std::shared_ptr<mi::InputChannel> const&)
    {
    }
};

struct NullInputTargeter : public msh::InputTargeter
{
    NullInputTargeter() = default;
    virtual ~NullInputTargeter() noexcept(true) = default;

    void focus_changed(std::shared_ptr<mi::InputChannel const> const&)
    {
    }

    void focus_cleared()
    {
    }
};

struct NullInputManager : public mi::InputManager
{
    void start()
    {
    }
    void stop()
    {
    }
    std::shared_ptr<mi::InputChannel> make_input_channel()
    {
        return std::shared_ptr<mi::InputChannel>();
    }
};

}

std::shared_ptr<ms::InputRegistrar> mi::NullInputConfiguration::the_input_registrar()
{
    return std::make_shared<NullInputRegistrar>();
}

std::shared_ptr<msh::InputTargeter> mi::NullInputConfiguration::the_input_targeter()
{
    return std::make_shared<NullInputTargeter>();
}

std::shared_ptr<mi::InputManager> mi::NullInputConfiguration::the_input_manager()
{
    return std::make_shared<NullInputManager>();
}

void mi::NullInputConfiguration::set_input_targets(std::shared_ptr<mi::InputTargets> const&)
{
}
