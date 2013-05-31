/*
 * Copyright (C) 2013 Canonical LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Robert Carr <robert.carr@canonical.com>
 */

// TODO: ~racarr. What license should this file be under?

#ifndef _UI_INPUT_ENUMERATOR_H
#define _UI_INPUT_ENUMERATOR_H

#include <std/RefBase.h>

#include <functional>

namespace android 
{
class InputWindowHandle;

class InputEnumerator : public RefBase {
public:
    virtual void for_each(std::function<void(sp<InputWindowHandle> const&)> const& callback) = 0;

protected:
    InputEnumerator() = default;
    virtual ~InputEnumerator() = default;
};

} // namespace android

#endif // _UI_INPUT_ENUMERATOR_H
