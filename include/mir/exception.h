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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_EXCEPTION_H_
#define MIR_EXCEPTION_H_

#include <boost/exception/all.hpp>

#include <exception>

#define MIR_THROW_EXCEPTION(e) BOOST_THROW_EXCEPTION(e)

namespace mir
{

using boost::error_info;

using boost::current_exception_diagnostic_information;
using boost::diagnostic_information;
using boost::diagnostic_information_what;
using boost::enable_error_info;
using boost::current_exception;

// Inject pre-defined error infos
using boost::errinfo_nested_exception;
using boost::errinfo_api_function;
using boost::errinfo_errno;
using boost::errinfo_file_handle;
using boost::errinfo_file_name;
using boost::errinfo_file_open_mode;

class Exception : public boost::exception, public std::exception
{
  public:

    template<typename ErrorInfoType>
    bool has_error_info() const
    {
        return error_info<ErrorInfoType>();
    }

    template<typename ErrorInfoType>
    typename ErrorInfoType::value_type const * error_info() const
    {
        return boost::get_error_info<ErrorInfoType>(*this);
    }

    template<typename ErrorInfoType>
    typename ErrorInfoType::value_type* error_info()
    {
        return boost::get_error_info<ErrorInfoType>(*this);
    }
};
}

#endif // MIR_EXCEPTION_H_
