/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_INPUT_ANDROID_CACHED_ANDROID_PTR_H
#define MIR_INPUT_ANDROID_CACHED_ANDROID_PTR_H

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

#include <functional>

namespace droidinput = android;
namespace mir
{
namespace input
{
namespace android
{
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

}
}
}

#endif

