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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_EXAMPLES_INPROCESS_EGL_CLIENT_H_
#define MIR_EXAMPLES_INPROCESS_EGL_CLIENT_H_

#include <thread>

namespace mir
{
class DisplayServer;

namespace examples
{

/// Encapsulation of a simple threaded client demonstrating inprocess EGL rendering
class InprocessEGLClient
{
public:
    InprocessEGLClient(mir::DisplayServer* server);
    virtual ~InprocessEGLClient() {}
    
    void start();
protected:
    InprocessEGLClient(InprocessEGLClient const&) = delete;
    InprocessEGLClient& operator=(InprocessEGLClient const&) = delete;

private:
    mir::DisplayServer* server;
    
    std::thread client_thread;
    void thread_loop();
};

}
} // namespace mir

#endif // MIR_EXAMPLES_INPROCESS_EGL_CLIENT_H_
