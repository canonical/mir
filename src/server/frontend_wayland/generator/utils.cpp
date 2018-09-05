/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include "utils.h"

#include <vector>
#include <string>
#include <locale>

const std::vector<std::string> cpp_reserved_keywords = {"namespace", "class", "default"}; // add to this on an as-needed basis

std::string file_name_from_path(std::string const& path)
{
    size_t i = path.find_last_of("/");
    if (i == std::string::npos)
        return path;
    else
        return path.substr(i + 1);
}

std::string sanitize_name(std::string const& name_, bool escape_invalid)
{
    std::string name = name_;
    if (name.empty())
        name = "__EMPTY_NAME";
    if (name[0] >= '0' && name[0] <= '9')
        name = "_" + name;
    for (unsigned long i = 0; i < name.size(); i++)
    {
        char c = name[i];
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            (c == '_'))
        {
            // all good
        }
        else
        {
            std::string replace_str = "_";
            if (escape_invalid)
                replace_str = "_ASC" + std::to_string((int)name[i]) + "_";
            name.replace(i, 1, replace_str);
        }
    }
    for (auto const& i: cpp_reserved_keywords)
    {
        if (i == name)
        {
            name = name + "_";
        }
    }
    return name;
}

std::string to_upper_case(std::string const& name)
{
    std::string macro_name = "";
    for (unsigned i = 0; i < name.size(); i++)
    {
        macro_name += std::toupper(name[i], std::locale("C"));
    }
    return sanitize_name(macro_name, false);
}

std::string to_camel_case(std::string const& name)
{
    std::string camel_cased_name;
    camel_cased_name = std::string{std::toupper(name[0], std::locale("C"))} + name.substr(1);
    auto next_underscore_offset = name.find('_');
    while (next_underscore_offset != std::string::npos)
    {
        if (next_underscore_offset < camel_cased_name.length())
        {
            camel_cased_name = camel_cased_name.substr(0, next_underscore_offset) +
                               std::toupper(camel_cased_name[next_underscore_offset + 1], std::locale("C")) +
                               camel_cased_name.substr(next_underscore_offset + 2);
        }
        next_underscore_offset = camel_cased_name.find('_', next_underscore_offset);
    }
    return sanitize_name(camel_cased_name);
}

