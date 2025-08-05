/*
* Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gdbus.h"

#include <mutex>

auto miral::gdbus::MainLoop::the_main_loop() -> std::shared_ptr<MainLoop>
{
    static std::weak_ptr<MainLoop> weak_loop;
    static std::mutex mutex;

    std::lock_guard lock{mutex};
    if (auto loop = weak_loop.lock())
    {
        return loop;
    }
    else
    {
        loop.reset(new MainLoop);
        weak_loop = loop;
        return loop;
    }
}

miral::gdbus::MainLoop::MainLoop() :
    loop{g_main_loop_new(NULL, FALSE)},
    t{[this](){ g_main_loop_run(loop); }}
{
}

miral::gdbus::MainLoop::~MainLoop()
{
    g_main_loop_quit(loop);
    g_main_loop_unref(loop);
}
