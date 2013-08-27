/*
 * Copyright © 2012, 2013 Canonical Ltd.
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

#include "mir/run_mir.h"
#include "mir/report_exception.h"
#include "server_configuration.h"

#include <iostream>

int main(int argc, char const* argv[])
try
{
    mir::examples::ServerConfiguration config(argc, argv);

    run_mir(config, [](mir::DisplayServer&) {/* empty init */});
    return 0;
}
catch (...)
{
    mir::report_exception(std::cerr);
    return 1;
}
