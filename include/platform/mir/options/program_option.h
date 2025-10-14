/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef MIR_OPTIONS_PROGRAM_OPTION_H_
#define MIR_OPTIONS_PROGRAM_OPTION_H_

#include <mir/options/option.h>

#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>

namespace mir
{
namespace options
{
class ProgramOption : public Option
{
public:
    ProgramOption();

    void parse_arguments(
        boost::program_options::options_description const& description,
        int argc,
        char const* argv[]);

    void parse_environment(
        boost::program_options::options_description const& description,
        std::function<std::string(std::string)> env_var_name_mapper);

    void parse_file(
        boost::program_options::options_description const& description,
        std::string const& filename);

    bool is_set(char const* name) const override;
    bool get(char const* name, bool default_) const override;
    std::string get(char const*, char const* default_) const override;
    int get(char const* name, int default_) const override;
    boost::any const& get(char const* name) const override;
    using Option::get;

    std::vector<std::string> unparsed_command_line() const;

private:
    boost::program_options::variables_map options;
    std::vector<std::string> unparsed_tokens;
};
}
}


#endif /* MIR_OPTIONS_PROGRAM_OPTION_H_ */
