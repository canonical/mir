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

#ifndef MIR_INPUT_ANDROID_COMMON_INPUT_THREAD_H_
#define MIR_INPUT_ANDROID_COMMON_INPUT_THREAD_H_

#include "android_input_thread.h"

#include <utils/StrongPointer.h>
#include <std/Thread.h>

#include <string>

namespace droidinput = android;

namespace  mir
{
namespace input
{
namespace android
{
class CommonInputThread : public InputThread
{
public:
    CommonInputThread(std::string const& name, droidinput::sp<droidinput::Thread> const& thread);
    void start();
    void request_stop();
    void join();

private:
    CommonInputThread(const CommonInputThread&) = delete;
    CommonInputThread& operator=(const CommonInputThread&) = delete;

    std::string const name;
    droidinput::sp<droidinput::Thread> const thread;
};

}
}
}

#endif
