/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_DISPLAY_SERVER_H_
#define MIR_DISPLAY_SERVER_H_

#include <memory>

/// All things Mir
namespace mir
{
class ServerConfiguration;

class DisplayServer
{
public:
    explicit DisplayServer(ServerConfiguration& config);

    ~DisplayServer();

    void run();
    void stop();

private:
    struct Private;
    std::unique_ptr<Private> const p;

    DisplayServer() = delete;
    DisplayServer(const DisplayServer&) = delete;
    DisplayServer& operator=(const DisplayServer&) = delete;
};

}

#endif /* MIR_DISPLAY_SERVER_H_ */
