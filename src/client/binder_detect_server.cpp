/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_client/detect_server.h"

#include "mir/chrono/chrono.h"
#include "mir/thread/all.h"

#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

//bool mir::client::detect_server(const std::string& name, std::chrono::milliseconds const& timeout)
//{
//    using std::chrono::system_clock::now;
//
//    auto limit = now() + timeout;
//
//    auto const& sm = android::defaultServiceManager();
//    android::String16 const service_name(name.c_str());
//
//    auto b = sm->checkService(service_name);
//    auto running = false;
//
//    while ((!b.get() || !running) && now() < limit)
//    {
//        if (b.get())
//        {
//            running = b->pingBinder() == android::OK;
//        }
//
//        if (!running)
//        {
//            std::this_thread::sleep_for(timeout/8);
//            b = sm->checkService(service_name);
//        }
//    }
//
//    return running;
//}

bool mir::client::detect_server(const std::string&, std::chrono::milliseconds const& timeout)
{
//    android::defaultServiceManager();
    std::this_thread::sleep_for(timeout/8);
    return true;
}
