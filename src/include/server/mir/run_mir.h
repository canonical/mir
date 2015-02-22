/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_RUN_MIR_H_
#define MIR_RUN_MIR_H_

#include <functional>
#include <iosfwd>

namespace mir
{
class ServerConfiguration;
class DisplayServer;

/**
 *  Run a DisplayServer with the supplied configuration.
 *  init will be called after constructing the server, but before invoking DisplayServer::run()
 *  The server will be stopped on receipt of SIGTERM or SIGINT
 *  This function does not return until the server has stopped.
 */
void run_mir(
    ServerConfiguration& config,
    std::function<void(DisplayServer&)> init);

/**
 *  Run a DisplayServer with the supplied configuration.
 *  init will be called after constructing the server, but before invoking DisplayServer::run()
 *  The terminator will be called following receipt of SIGTERM or SIGINT
 *  (but not in a signal handler - so arbitrary functions may be invoked).
 *  This function does not return until the server has stopped.
 */
void run_mir(
    ServerConfiguration& config,
    std::function<void(DisplayServer&)> init,
    std::function<void(int)> const& terminator);
}

#endif /* MIR_RUN_MIR_H_ */
