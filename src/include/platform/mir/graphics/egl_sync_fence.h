/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_EGL_SYNC_FENCE_H_
#define MIR_GRAPHICS_EGL_SYNC_FENCE_H_

#include "egl_extensions.h"
#include "command_stream_sync.h"
#include <memory>
#include <mutex>

namespace mir
{
namespace graphics
{
class NullCommandSync : public CommandStreamSync
{
    void raise() override;
    void reset() override;
    bool wait_for(std::chrono::nanoseconds ns) override;
};

class EGLSyncFence : public CommandStreamSync
{
public:
    EGLSyncFence(std::shared_ptr<EGLSyncExtensions> const&);
    ~EGLSyncFence();

    void raise() override;
    void reset() override;
    bool wait_for(std::chrono::nanoseconds ns) override;
private:
    void reset(std::unique_lock<std::mutex> const&);
    bool wait_for(std::unique_lock<std::mutex> const&, std::chrono::nanoseconds ns);

    std::shared_ptr<EGLSyncExtensions> const egl;
    std::chrono::nanoseconds const default_timeout{
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(1))};

    std::mutex mutex;
    EGLDisplay fence_display;
    EGLSyncKHR sync_point;
};

}
}

#endif /* MIR_GRAPHICS_EGL_SYNC_FENCE_H_ */
