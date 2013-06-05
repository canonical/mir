/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/lttng/tracepoint_provider.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

#include <dlfcn.h>

mir::lttng::TracepointProvider::TracepointProvider(std::string const& lib_name)
    : lib{dlopen(lib_name.c_str(), RTLD_NOW)}
{
    if (lib == nullptr)
    {
        std::string msg{"Failed to load tracepoint provider: "};
        msg += dlerror();
        BOOST_THROW_EXCEPTION(std::runtime_error(msg));
    }
}

mir::lttng::TracepointProvider::~TracepointProvider() noexcept
{
    dlclose(lib);
}
