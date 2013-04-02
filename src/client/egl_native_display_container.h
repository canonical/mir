/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_CLIENT_GBM_EGL_NATIVE_DISPLAY_CONTAINER_H_
#define MIR_CLIENT_GBM_EGL_NATIVE_DISPLAY_CONTAINER_H_

#include "mir_toolkit/c_types.h"

namespace mir
{
namespace client
{

class EGLNativeDisplayContainer
{
public:
    virtual ~EGLNativeDisplayContainer() {}

    virtual mir_toolkit::MirEGLNativeDisplayType create(mir_toolkit::MirConnection* connection) = 0;
    virtual void release(mir_toolkit::MirEGLNativeDisplayType display) = 0;

    virtual bool validate(mir_toolkit::MirEGLNativeDisplayType display) const = 0;

    static EGLNativeDisplayContainer& instance();

protected:
    EGLNativeDisplayContainer() = default;
    EGLNativeDisplayContainer(EGLNativeDisplayContainer const&) = delete;
    EGLNativeDisplayContainer& operator=(EGLNativeDisplayContainer const&) = delete;
};

}
} // namespace mir

#endif // MIR_CLIENT_GBM_EGL_NATIVE_DISPLAY_CONTAINER_H_
