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

#include "mir/lttng/tracepoint_provider.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

#include <dlfcn.h>

namespace
{
char const* const tracepoint_provider_library = "libmirserverlttng.so";
}

mir::lttng::TracepointProvider::TracepointProvider()
    : lib{dlopen(tracepoint_provider_library, RTLD_NOW)}
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
