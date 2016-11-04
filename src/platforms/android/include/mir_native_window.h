/*
 * Copyright © 2012 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_ANDROID_MIR_NATIVE_WINDOW_H_
#define MIR_GRAPHICS_ANDROID_MIR_NATIVE_WINDOW_H_

#include <system/window.h>
#include <cstdarg>
#include <memory>
#include <vector>

namespace mir
{
namespace graphics
{
namespace android
{
class AndroidDriverInterpreter;
class SyncFileOps;
class NativeWindowReport;

class MirNativeWindow : public ANativeWindow
{
public:
    explicit MirNativeWindow(
        std::shared_ptr<AndroidDriverInterpreter> const& interpreter,
        std::shared_ptr<NativeWindowReport> const& report);

    int query(int key, int* value) const;
    int perform(int key, va_list args );
    int dequeueBuffer(struct ANativeWindowBuffer** buffer, int* fence);
    int dequeueBufferAndWait(struct ANativeWindowBuffer** buffer);
    int queueBuffer(struct ANativeWindowBuffer* buffer, int fence);
    int queueBufferDeprecated(struct ANativeWindowBuffer* buffer);
    int cancelBuffer(struct ANativeWindowBuffer* buffer, int fence);
    int cancelBufferDeprecated(struct ANativeWindowBuffer* buffer);
    int setSwapInterval(int interval);
    void use_native_surface(std::shared_ptr<AndroidDriverInterpreter> const& interpreter);
private:
    std::shared_ptr<AndroidDriverInterpreter> driver_interpreter;
    std::shared_ptr<NativeWindowReport> const report;
    std::shared_ptr<SyncFileOps> const sync_ops;
    std::vector<struct ANativeWindowBuffer*> cancelled_buffers;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_MIR_NATIVE_WINDOW_H_ */
