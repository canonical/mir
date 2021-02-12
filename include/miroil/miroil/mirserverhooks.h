/*
 * Copyright © 2021 Canonical Ltd.
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
 */

#ifndef MIROIL_MIRSERVERHOOKS_H
#define MIROIL_MIRSERVERHOOKS_H
#include "miroil/inputdeviceobserver.h"
#include "miroil/promptsessionlistener.h"
#include <functional>
#include <memory>

namespace mir { class Server; }
namespace mir { namespace scene { class PromptSessionManager; } }
namespace mir { namespace graphics { class Display; } }
namespace mir { namespace input { class InputDeviceHub; } }
namespace mir { namespace shell { class DisplayConfigurationController; } }

namespace miroil
{
class MirServerHooks
{
public:
    MirServerHooks(std::function<std::shared_ptr<miroil::PromptSessionListener>()> createListener);

    void operator()(mir::Server& server);

    miroil::PromptSessionListener * promptSessionListener() const;
    std::shared_ptr<mir::scene::PromptSessionManager> thePromptSessionManager() const;
    std::shared_ptr<mir::graphics::Display> theMirDisplay() const;
    std::shared_ptr<mir::input::InputDeviceHub> theInputDeviceHub() const;
    std::shared_ptr<mir::shell::DisplayConfigurationController> theDisplayConfigurationController() const;    

    void createInputDeviceObserver(std::shared_ptr<miroil::InputDeviceObserver> const& observer);

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIROI_MIRSERVERHOOKS_H
