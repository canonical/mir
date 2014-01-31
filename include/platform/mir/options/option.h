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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_OPTIONS_OPTION_H_
#define MIR_OPTIONS_OPTION_H_

#include <boost/any.hpp>

#include <string>

namespace mir
{

/// System options. Interface for extracting configuration options from wherever
/// they may be (e.g. program arguments, config files or environment variables).
namespace options
{
class Option
{
public:
    virtual bool is_set(char const* name) const = 0;

    virtual bool get(char const* name, bool default_) const = 0;
    virtual std::string get(char const* name, char const* default_) const = 0;
    virtual int get(char const* name, int default_) const = 0;
    virtual boost::any const& get(char const* name) const = 0;

    template<typename Type>
    Type get(char const* name) const
    { return boost::any_cast<Type>(get(name)); }

protected:
    Option() = default;
    virtual ~Option() = default;
    Option(Option const&) = delete;
    Option& operator=(Option const&) = delete;
};
}
}


#endif /* MIR_OPTIONS_OPTION_H_ */
