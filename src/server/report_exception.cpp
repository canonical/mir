/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir/report_exception.h"
#include "mir/abnormal_exit.h"

#include <boost/exception/diagnostic_information.hpp>

#include <iostream>

void mir::report_exception(std::ostream& out)
{
    try
    {
        throw;
    }
    catch (mir::AbnormalExit const& error)
    {
        out << error.what() << std::endl;
    }
    catch (std::exception const& error)
    {
        out << "ERROR: " << boost::diagnostic_information(error) << std::endl;
    }
    catch (...)
    {
        out << "ERROR: unrecognised exception. (This is weird!)" << std::endl;
    }
}

void mir::report_exception()
{
    report_exception(std::cerr);
}
