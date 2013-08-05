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
      focus(focus)
{
}

std::shared_ptr<mg::DisplayConfiguration> msh::MediatingDisplayChanger::active_configuration()
{
    return display->configuration();
}

void msh::MediatingDisplayChanger::configure(
    std::weak_ptr<mf::Session> const& requesting_application,
    std::shared_ptr<mg::DisplayConfiguration> const& requested_configuration)
{
    if ( requesting_application.lock() != focus->focussed_application().lock())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("display change request denied, application does not have focus"));
    }
    else
    {
        compositor->stop();
        display->configure(*requested_configuration);
        compositor->start();
    }
}
