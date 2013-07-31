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
#include "mir/graphics/display.h"
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mg = mir::graphics;

msh::MediatingDisplayChanger::MediatingDisplayChanger(std::shared_ptr<mg::Display> const& display)
    : display(display)
{
}

std::shared_ptr<mg::DisplayConfiguration> msh::MediatingDisplayChanger::active_configuration()
{
    return display->configuration();
}

void msh::MediatingDisplayChanger::configure(
    std::weak_ptr<mf::Session> const&, std::shared_ptr<mg::DisplayConfiguration> const&)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("TODO: display changing not implemented!"));
}
