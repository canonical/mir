// Copyright (C) 2010 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// Generic key layout file for full alphabetic US English PC style
// external keyboards.  This file is intentionally very generic and is
// intended to support a broad rang of keyboards.  Do not edit the
// generic key layout to support a specific keyboard; instead, create
// a new key layout file with the required keyboard configuration.
// Taken from android source tree Generic.kl and Generic.kcm

#ifndef GENERIC_KEY_MAP_H_
#define GENERIC_KEY_MAP_H_

namespace android
{
struct GenericKeyMap
{
    static const char* key_layout_contents();

    static const char* keymap_contents();
};
}

#endif // GENERIC_KEY_MAP_H_
