/*
 * Copyright (C) 2005 The Android Open Source Project
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
 */

#ifndef ANDROID_TYPE_HELPERS_H
#define ANDROID_TYPE_HELPERS_H

#include <new>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

// ---------------------------------------------------------------------------

namespace android {

/*
 * compare and order types
 */

template<typename TYPE> inline
int strictly_order_type(const TYPE& lhs, const TYPE& rhs) {
    return (lhs < rhs) ? 1 : 0;
}

template<typename TYPE> inline
int compare_type(const TYPE& lhs, const TYPE& rhs) {
    return strictly_order_type(rhs, lhs) - strictly_order_type(lhs, rhs);
}

// ---------------------------------------------------------------------------

/*
 * a key/value pair
 */

template <typename KEY, typename VALUE>
struct key_value_pair_t {
    typedef KEY key_t;
    typedef VALUE value_t;

    KEY     key;
    VALUE   value;
    key_value_pair_t() { }
    key_value_pair_t(const key_value_pair_t& o) : key(o.key), value(o.value) { }
    key_value_pair_t(const KEY& k, const VALUE& v) : key(k), value(v)  { }
    key_value_pair_t(const KEY& k) : key(k) { }
    inline bool operator < (const key_value_pair_t& o) const {
        return strictly_order_type(key, o.key);
    }
    inline const KEY& getKey() const {
        return key;
    }
    inline const VALUE& getValue() const {
        return value;
    }
    friend bool operator == (const key_value_pair_t& lhs, const key_value_pair_t& rhs) {
        return !compare_type(lhs.key, rhs.key);
    }
};


} // namespace android

// ---------------------------------------------------------------------------

#endif // ANDROID_TYPE_HELPERS_H
