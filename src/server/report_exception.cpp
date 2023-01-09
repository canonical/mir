/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/report_exception.h"
#include "mir/abnormal_exit.h"

#include <boost/exception/diagnostic_information.hpp>

#include <iostream>

void mir::report_exception(std::ostream& out_stream, std::ostream& err_stream)
{
    try
    {
        throw;
    }
    catch (ExitWithOutput const& output)
    {
        out_stream << output.what() << std::endl;
    }
    catch (AbnormalExit const& error)
    {
        err_stream << error.what() << std::endl;
    }
    catch (std::exception const& error)
    {
        // First report any nested exceptions
        try
        {
            std::rethrow_if_nested(error);
        }
        catch(...)
        {
            report_exception(out_stream, err_stream);
        }

        err_stream << "ERROR: " << boost::diagnostic_information(error) << std::endl;
    }
    catch (...)
    {
        err_stream << "ERROR: unrecognised exception. (This is weird!)" << std::endl;
    }
}

void mir::report_exception(std::ostream& stream)
{
    report_exception(stream, stream);
}

void mir::report_exception()
{
    report_exception(std::cout, std::cerr);
}
