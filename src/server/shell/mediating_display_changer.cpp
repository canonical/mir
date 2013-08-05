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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/shell/mediating_display_changer.h"
#include "mir/shell/focus_controller.h"
#include "mir/shell/session.h"
#include "mir/compositor/compositor.h"
#include "mir/graphics/display.h"
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mc = mir::compositor;

msh::MediatingDisplayChanger::MediatingDisplayChanger(
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mc::Compositor> const& compositor,
    std::shared_ptr<msh::FocusController> const& focus)
    : display(display),
      compositor(compositor),
      focus(focus),
      base_config(display->configuration()),
      active_config(base_config)
{
}

std::shared_ptr<mg::DisplayConfiguration> msh::MediatingDisplayChanger::active_configuration()
{
    return active_config;
}

void msh::MediatingDisplayChanger::apply_config(
    std::shared_ptr<mg::DisplayConfiguration> const& requested_configuration)
{
    compositor->stop();
    display->configure(*requested_configuration);
    active_config = requested_configuration;
    compositor->start();
}

void msh::MediatingDisplayChanger::configure(
    std::weak_ptr<mf::Session> const& app,
    std::shared_ptr<mg::DisplayConfiguration> const& requested_configuration)
{
    auto requesting_application = app.lock();
    config_map[requesting_application.get()] = requested_configuration;

    if ( requesting_application == focus->focussed_application().lock())
    {
        apply_config(requested_configuration);
    }
}

void msh::MediatingDisplayChanger::set_focus_to(std::weak_ptr<mf::Session> const& app)
{
    auto it = config_map.find(app.lock().get());
    if (it == config_map.end())
    {
        apply_config(base_config);
    }
    else
    {
        apply_config(it->second);
    }
}

void msh::MediatingDisplayChanger::remove_configuration_for(std::weak_ptr<mf::Session> const& app)
{
    auto it = config_map.find(app.lock().get());
    if (it != config_map.end())
    {
        config_map.erase(it);
        if (it->second == active_config)
        {
            apply_config(base_config);
        }
    }

}
