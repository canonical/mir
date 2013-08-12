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
#include "mir/compositor/compositor.h"
#include "mir/input/input_manager.h"

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mi = mir::input;

namespace
{

class ApplyNowAndRevertOnScopeExit
{
public:
    ApplyNowAndRevertOnScopeExit(std::function<void()> const& apply,
                                 std::function<void()> const& revert)
        : revert{revert}
    {
        apply();
    }

    ~ApplyNowAndRevertOnScopeExit()
    {
        revert();
    }

private:
    ApplyNowAndRevertOnScopeExit(ApplyNowAndRevertOnScopeExit const&) = delete;
    ApplyNowAndRevertOnScopeExit& operator=(ApplyNowAndRevertOnScopeExit const&) = delete;

    std::function<void()> const revert;
};

}

msh::MediatingDisplayChanger::MediatingDisplayChanger(
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mc::Compositor> const& compositor,
    std::shared_ptr<mi::InputManager> const& input_manager)
    : display{display},
      compositor{compositor},
      input_manager{input_manager}
{
}

void msh::MediatingDisplayChanger::configure(
    std::weak_ptr<mf::Session> const&,
    std::shared_ptr<mg::DisplayConfiguration> const& conf)
{
    ApplyNowAndRevertOnScopeExit comp{
        [this] { compositor->stop(); },
        [this] { compositor->start(); }};

    ApplyNowAndRevertOnScopeExit input{
        [this] { input_manager->stop(); },
        [this] { input_manager->start(); }};

    display->configure(*conf);
}

std::shared_ptr<mg::DisplayConfiguration>
msh::MediatingDisplayChanger::active_configuration()
{
    return display->configuration();
}
