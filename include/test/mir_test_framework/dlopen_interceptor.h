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

#ifndef MIR_TEST_FRAMEWORK_DLOPEN_INTERCEPTOR_H_
#define MIR_TEST_FRAMEWORK_DLOPEN_INTERCEPTOR_H_

#include <functional>
#include <optional>
#include <string>
#include <memory>
#include <dlfcn.h>

namespace mir_test_framework
{
class DlopenInterposerHandle
{
public:
    class Handle;

    ~DlopenInterposerHandle();
    DlopenInterposerHandle(std::unique_ptr<Handle> handle);
private:
    std::unique_ptr<Handle> handle;
};

using DLopenFilter =
    std::function<
        std::optional<void*>(
            decltype(&dlopen) real_dlopen,
            char const* filename, int flags)>;

DlopenInterposerHandle add_dlopen_filter(DLopenFilter filter);
}

#endif //MIR_TEST_FRAMEWORK_DLOPEN_INTERCEPTOR_H_
