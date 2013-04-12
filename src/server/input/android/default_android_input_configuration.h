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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_ANDROID_DEFAULT_ANDROID_INPUT_CONFIGURATION_H_
#define MIR_INPUT_ANDROID_DEFAULT_ANDROID_INPUT_CONFIGURATION_H_

#include "android_input_configuration.h"

#include "mir/cached_ptr.h"

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

#include <functional>

namespace droidinput = android;

namespace android
{
class InputReaderInterface;
class InputReaderPolicyInterface;
class InputDispatcherPolicyInterface;
}

namespace mir
{
namespace graphics
{
class ViewableArea;
}
namespace input
{
class EventFilter;
class EventFilterChain;
class CursorListener;

namespace android
{

class DefaultInputConfiguration : public InputConfiguration
{
public:
    DefaultInputConfiguration(std::initializer_list<std::shared_ptr<EventFilter> const> const& filters,
                              std::shared_ptr<graphics::ViewableArea> const& view_area,
                              std::shared_ptr<CursorListener> const& cursor_listener);
    virtual ~DefaultInputConfiguration();

    droidinput::sp<droidinput::EventHubInterface> the_event_hub();
    droidinput::sp<droidinput::InputDispatcherInterface> the_dispatcher();
    droidinput::sp<droidinput::InputReaderInterface> the_reader();

    std::shared_ptr<InputThread> the_dispatcher_thread();
    std::shared_ptr<InputThread> the_reader_thread();

    virtual droidinput::sp<droidinput::InputDispatcherPolicyInterface> the_dispatcher_policy();
    virtual droidinput::sp<droidinput::InputReaderPolicyInterface> the_reader_policy();

protected:
    DefaultInputConfiguration(DefaultInputConfiguration const&) = delete;
    DefaultInputConfiguration& operator=(DefaultInputConfiguration const&) = delete;

private:
    template <typename Type>
    class CachedAndroidPtr
    {
        droidinput::wp<Type> cache;

        CachedAndroidPtr(CachedAndroidPtr const&) = delete;
        CachedAndroidPtr& operator=(CachedAndroidPtr const&) = delete;

    public:
        CachedAndroidPtr() = default;

        droidinput::sp<Type> operator()(std::function<droidinput::sp<Type>()> make)
        {
            auto result = cache.promote();
            if (!result.get())
            {
                cache = result = make();
            }
            return result;
        }
    };

    std::shared_ptr<EventFilterChain> const filter_chain;
    std::shared_ptr<graphics::ViewableArea> const view_area;
    std::shared_ptr<CursorListener> const cursor_listener;

    CachedPtr<InputThread> dispatcher_thread;
    CachedPtr<InputThread> reader_thread;
    CachedAndroidPtr<droidinput::EventHubInterface> event_hub;
    CachedAndroidPtr<droidinput::InputDispatcherPolicyInterface> dispatcher_policy;
    CachedAndroidPtr<droidinput::InputReaderPolicyInterface> reader_policy;
    CachedAndroidPtr<droidinput::InputDispatcherInterface> dispatcher;
    CachedAndroidPtr<droidinput::InputReaderInterface> reader;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_DEFAULT_ANDROID_INPUT_CONFIGURATION_H_
