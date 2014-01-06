/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/gl_extensions_base.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

#include <cstring>

namespace mg = mir::graphics;

mg::GLExtensionsBase::GLExtensionsBase(char const* extensions)
    : extensions{extensions}
{
    if (!extensions)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Couldn't get list of GL extensions"));
    }
}

bool mg::GLExtensionsBase::support(char const* ext) const
{
    char const* ext_ptr = extensions;
    size_t const len = strlen(ext);

    while ((ext_ptr = strstr(ext_ptr, ext)) != nullptr)
    {
        if (ext_ptr[len] == ' ' || ext_ptr[len] == '\0')
            break;
        ext_ptr += len;
    }

    return ext_ptr != nullptr;
}
