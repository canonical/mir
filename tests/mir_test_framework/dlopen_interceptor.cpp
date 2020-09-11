/*
 * Copyright © 2020 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir_test_framework/dlopen_interceptor.h"

#include <dlfcn.h>
#include <cstring>
#include <cassert>
#include <vector>
#include <mutex>
#include <algorithm>

namespace mtf = mir_test_framework;

class mtf::DlopenInterposerHandle::Handle
{
public:
    Handle(DLopenFilter filter)
        : filter{std::move(filter)}
    {
    }

    auto apply(decltype(&dlopen) real_dlopen, char const* filename, int flags) -> std::optional<void*>
    {
        return filter(real_dlopen, filename, flags);
    }
private:
    DLopenFilter filter;
};

namespace
{
std::mutex interposer_mutex;
std::vector<mtf::DlopenInterposerHandle::Handle*> interposers;
}

mtf::DlopenInterposerHandle::DlopenInterposerHandle(std::unique_ptr<Handle> handle)
    : handle{std::move(handle)}
{
    std::lock_guard<std::mutex> lock{interposer_mutex};
    interposers.push_back(this->handle.get());
}

mtf::DlopenInterposerHandle::~DlopenInterposerHandle()
{
    std::lock_guard<std::mutex> lock{interposer_mutex};
    interposers.erase(
        std::remove_if(
            interposers.begin(),
            interposers.end(),
            [raw_handle = handle.get()](auto candidate) { return candidate == raw_handle;}),
        interposers.end());
}

auto mtf::add_dlopen_filter(DLopenFilter filter) -> DlopenInterposerHandle
{
    auto handle = std::make_unique<mtf::DlopenInterposerHandle::Handle>(std::move(filter));

    return DlopenInterposerHandle(std::move(handle));
}

/*
 * Interposing implementation of dlopen
 */
void* dlopen(char const* filename, int flags)
{
    void* (*real_dlopen)(const char *filename, int flag) noexcept;
    real_dlopen = reinterpret_cast<decltype(real_dlopen)>(dlsym(RTLD_NEXT, "dlopen"));
    assert(real_dlopen);

    std::lock_guard<std::mutex> lock{interposer_mutex};
    std::optional<void*> interposed_result;
    for (auto const handle : interposers)
    {
        if (!interposed_result)
        {
            interposed_result = handle->apply(real_dlopen, filename, flags);
            // Don't exit the loop, to check if some other filter *also* wants to override this
        }
        else if (handle->apply(real_dlopen, filename, flags))
        {
            // Oops! Two of our handlers want to handle this; let's be friendly to debugging, and just abort.
            abort();
        }
    }
    return interposed_result.value_or(real_dlopen(filename, flags));
}
