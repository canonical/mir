/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/graphics/display.h"
#include "mir/shell/nullshell.h"

using namespace mir;
using namespace mir::graphics;
using namespace mir::shell;

Display::Display() :
    shell_running(false)
{
}

Display::~Display()
{
    if (shell_running)
        stop_shell();
}

void Display::set_shell(std::shared_ptr<mir::Shell> s)
{
    stop_shell();
    shell = s;
}

bool Display::start_shell()
{
    if (!shell)
        set_shell(std::make_shared<NullShell>());

    shell_running = shell->start();
    return shell_running;
}

void Display::stop_shell()
{
    if (shell_running)
    {
        shell->stop();
        shell_running = false;
    }
}
