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

#include "common_input_thread.h"

#include <std/Thread.h>

namespace mia = mir::input::android;

mia::CommonInputThread::CommonInputThread(std::string const& name, droidinput::sp<droidinput::Thread> const& thread)
      : name(name),
        thread(thread)
{
}

void mia::CommonInputThread::start()
{
    thread->run(name.c_str(), droidinput::PRIORITY_URGENT_DISPLAY);
}
void mia::CommonInputThread::request_stop()
{
    thread->requestExit();
}

void mia::CommonInputThread::join()
{
    thread->join();
}

