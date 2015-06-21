/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */


#ifndef MIR_TEST_POPEN_H_
#define MIR_TEST_POPEN_H_

#include <cstdio>
#include <memory>
#include <string>

namespace mir
{
namespace test
{
/**
 *  Popen - A popen c++ wrapper
 */
class Popen
{
public:
    Popen(std::string const& cmd);

    /**
     * Read a line from the output of the executed command
     * returns false if there is nothing more to read
     */
    bool get_line(std::string& line);

private:
    Popen() = delete;
    Popen(Popen const&) = delete;
    Popen& operator=(Popen const&) = delete;
    std::unique_ptr<std::FILE, void(*)(FILE* f)> raw_stream;
};

}
}

#endif
