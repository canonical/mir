/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_ANDROID_NATIVE_WINDOW_REPORT_H_
#define MIR_GRAPHICS_ANDROID_NATIVE_WINDOW_REPORT_H_
#include <system/window.h>
#include <vector>
#include <memory>

namespace mir
{
namespace logging
{
class Logger;
}
namespace graphics
{
namespace android
{
enum class BufferEvent
{
    Queue,
    Dequeue,
    Cancel
};
class NativeWindowReport
{
public:
    NativeWindowReport() = default;
    virtual ~NativeWindowReport() = default;

    virtual void buffer_event(BufferEvent type, ANativeWindow const* win, ANativeWindowBuffer* buf, int fence) const = 0;
    virtual void buffer_event(BufferEvent type, ANativeWindow const* win, ANativeWindowBuffer* buf) const = 0;
    virtual void query_event(ANativeWindow const* win, int type, int result) const = 0;
    virtual void perform_event(ANativeWindow const* win, int type, std::vector<int> const& args) const = 0;

private:
    NativeWindowReport(NativeWindowReport const&) = delete;
    NativeWindowReport& operator=(NativeWindowReport const&) = delete;
};

class NullNativeWindowReport : public NativeWindowReport
{
    void buffer_event(BufferEvent type, ANativeWindow const* win, ANativeWindowBuffer* buf, int fence) const override;
    void buffer_event(BufferEvent type, ANativeWindow const* win, ANativeWindowBuffer* buf) const override;
    void query_event(ANativeWindow const* win, int type, int result) const override;
    void perform_event(ANativeWindow const* win, int type, std::vector<int> const& args) const override;
};

class ConsoleNativeWindowReport : public NativeWindowReport
{
public:
    ConsoleNativeWindowReport(std::shared_ptr<logging::Logger> const&);
    void buffer_event(BufferEvent type, ANativeWindow const* win, ANativeWindowBuffer* buf, int fence) const override;
    void buffer_event(BufferEvent type, ANativeWindow const* win, ANativeWindowBuffer* buf) const override;
    void query_event(ANativeWindow const* win, int type, int result) const override;
    void perform_event(ANativeWindow const* win, int type, std::vector<int> const& args) const override;
private:
    std::shared_ptr<logging::Logger> const logger;
    std::string const component_name = "AndroidNativeWindow";
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_NATIVE_WINDOW_REPORT_H_ */
