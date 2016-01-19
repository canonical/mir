/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SHARED_LIBRARY_H_
#define MIR_SHARED_LIBRARY_H_

#include <string>

namespace mir
{
class SharedLibrary
{
public:
    explicit SharedLibrary(char const* library_name);
    explicit SharedLibrary(std::string const& library_name);
    ~SharedLibrary() noexcept;

    template<typename FunctionPtr>
    FunctionPtr load_function(char const* function_name) const
    {
        FunctionPtr result{};
        (void*&)result = load_symbol(function_name);
        return result;
    }

    template<typename FunctionPtr>
    FunctionPtr load_function(std::string const& function_name) const
    {
        return load_function<FunctionPtr>(function_name.c_str());
    }

    template<typename FunctionPtr>
    FunctionPtr load_function(std::string const& function_name, std::string const& version) const
    {
        FunctionPtr result{};
        (void*&)result = load_symbol(function_name.c_str(), version.c_str());
        return result;
    }
private:
    void* const so;
    void* load_symbol(char const* function_name) const;
    void* load_symbol(char const* function_name, char const* version) const;
    SharedLibrary(SharedLibrary const&) = delete;
    SharedLibrary& operator=(SharedLibrary const&) = delete;
};
}


#endif /* MIR_SHARED_LIBRARY_H_ */
