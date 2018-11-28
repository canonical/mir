/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORM_EGLSTREAM_KMS_UTILS_H_
#define MIR_PLATFORM_EGLSTREAM_KMS_UTILS_H_

#include <epoxy/egl.h>
#include <sys/types.h>
#include <experimental/optional>

namespace mir
{
namespace graphics
{
namespace eglstream
{
dev_t devnum_for_device(EGLDeviceEXT device);

struct VersionInfo
{
    int major;
    int minor;
};

std::experimental::optional<VersionInfo> parse_nvidia_version(char const* gl_version);
}
}
}

#endif //MIR_PLATFORM_EGLSTREAM_KMS_UTILS_H_
