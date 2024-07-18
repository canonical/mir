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

#include <system_error>
#include <unistd.h>

#include "mir/test/file_utils.h"

std::string mir::test::create_temp_file()
{
    char temp_file[] = "/tmp/temp_file_XXXXXX";
    if (int fd = mkstemp(temp_file) == -1)
    {
        throw std::system_error(errno, std::system_category(), "Failed to create temp file");
    }
    else
    {
        close(fd);
    }

    return temp_file;
}

std::string mir::test::create_temp_dir()
{
    char temp_dir[] = "/tmp/temp_dir_XXXXXX";
    if (mkdtemp(temp_dir) == nullptr)
        throw std::system_error(errno, std::system_category(), "Failed to create temp dir");

    return temp_dir;
}
