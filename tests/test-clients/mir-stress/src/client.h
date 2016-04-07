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
 * Authored by: Thomi Richards <thomi.richards@canonical.com>
 */

#ifndef MIR_STRESS_TEST_CLIENT_H_
#define MIR_STRESS_TEST_CLIENT_H_

#include <memory>


/// A simple state machine that knows how to connect to a mir server,
/// issue some devices, and exit cleanly.
class ClientStateMachine
{
public:
    typedef std::shared_ptr<ClientStateMachine> Ptr;

    virtual ~ClientStateMachine() {};

    static Ptr Create();

    virtual bool connect(std::string unique_name, const char* socket_file=0) =0;
    virtual bool create_surface() =0;
    virtual void release_surface() =0;
    virtual void disconnect() =0;
};


struct MirConnection;
struct MirSurface;

class UnacceleratedClient: public ClientStateMachine
{
public:
    UnacceleratedClient();
    bool connect(std::string unique_name, const char* socket_file=0) override;
    bool create_surface() override;
    void release_surface() override;
    void disconnect() override;
private:
    MirConnection* connection_;
    MirSurface* surface_;
};

#endif
