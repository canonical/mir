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

#include "default_input_manager.h"
#include "mir/input/platform.h"

#include "mir/main_loop.h"
#include "mir/thread_name.h"

namespace mi = mir::input;

mi::DefaultInputManager::DefaultInputManager(std::shared_ptr<Platform> const& input_platform,
                                             std::shared_ptr<InputDeviceRegistry> const& registry,
                                             std::shared_ptr<MainLoop> const& input_event_loop)
    : input_platform(input_platform), input_event_loop(input_event_loop), input_device_registry(registry), input_handler_register(input_event_loop)
{
    std::cerr << __FUNCTION__ << std::endl;
}

mi::DefaultInputManager::~DefaultInputManager()
{
    std::cerr << __FUNCTION__ << std::endl;
    stop();
}

void mi::DefaultInputManager::start()
{
    std::cerr << __PRETTY_FUNCTION__ << __LINE__ << std::endl;
    input_thread.reset(new std::thread(
        [this]()
        {
            std::cerr << __PRETTY_FUNCTION__ << __LINE__ << std::endl;
            mir::set_thread_name("Mir/Input");
            input_platform->start(input_handler_register, input_device_registry);
            auto cp = input_platform;
            input_event_loop->run();
            cp->stop(input_handler_register);
        }));
}

void mi::DefaultInputManager::stop()
{
    std::cerr << __PRETTY_FUNCTION__ << std::endl;
    input_event_loop->stop();
    if (input_thread &&
        input_thread->joinable())
    {
        input_thread->join();
    }
}

