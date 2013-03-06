/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_logger.h"

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/null.hpp>

#include <iostream>

#include <cstdlib>

namespace mcl = mir::client;


std::ostream& mcl::ConsoleLogger::error()
{
    return std::cerr  << "ERROR: ";
}

std::ostream& mcl::ConsoleLogger::debug()
{
    static char const* const debug = getenv("MIR_CLIENT_DEBUG");

    if (debug) return std::cerr  << "DEBUG: ";

    static boost::iostreams::stream<boost::iostreams::null_sink> null((boost::iostreams::null_sink()));
    return null;
}

std::ostream& mcl::NullLogger::error()
{
    static boost::iostreams::stream<boost::iostreams::null_sink> null((boost::iostreams::null_sink()));
    return null;
}

std::ostream& mcl::NullLogger::debug()
{
    static boost::iostreams::stream<boost::iostreams::null_sink> null((boost::iostreams::null_sink()));
    return null;
}
