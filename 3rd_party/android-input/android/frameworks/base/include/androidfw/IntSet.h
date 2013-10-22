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
 * Author: Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

// TODO: ~dandrader. What license should this file be under?

#ifndef ANDROID_INTSET_H
#define ANDROID_INTSET_H

#include <assert.h>

// C++ std lib
#include <algorithm>
#include <sstream>
#include <set>

namespace android {

/*
  A set of integers

  It serves two purposes:
   - Provide a convenience wrapper for std::set<int32_t>. Because the std API is cumbersome.
   - Provide an API similar to the BitSet32 class that it's replacing.
 */
class IntSet : public std::set<int32_t> {
public:

#ifdef ANDROID_INPUT_INTSET_TEST
    static int constructionCount;
    static int destructionCount;
#endif

    IntSet() : std::set<int32_t>() {
#ifdef ANDROID_INPUT_INTSET_TEST
        ++constructionCount;
#endif
    }

    IntSet(std::initializer_list<int32_t> list)
        : std::set<int32_t>(list) {
#ifdef ANDROID_INPUT_INTSET_TEST
        ++constructionCount;
#endif
    }

    virtual ~IntSet() {
#ifdef ANDROID_INPUT_INTSET_TEST
        ++destructionCount;
#endif
    }

    IntSet operator -(const IntSet &other) const {
        IntSet result;

        std::set_difference(cbegin(), cend(),
                other.cbegin(), other.cend(),
                std::inserter(result, result.begin()));

        return result;
    }

    IntSet operator &(const IntSet &other) const {
        IntSet result;

        std::set_intersection(cbegin(), cend(),
                         other.cbegin(), other.cend(),
                         std::inserter(result, result.begin()));

        return result;
    }

    template<typename Func>
    void forEach(Func func) { std::for_each(begin(), end(), func); }

    template<typename Func>
    void forEach(Func func) const { std::for_each(begin(), end(), func); }

    void remove(int32_t value) { erase(value); }

    void remove(const IntSet &values) {
        remove(begin(), values.begin(), values.end());
    }

    size_t count() const { return size(); }

    bool isEmpty() const { return empty(); }

    bool contains(int32_t value) const { return find(value) != end(); }

    int32_t first() const { return *cbegin(); }

    // It's assumed that the given value does exist in the set
    size_t indexOf(int32_t value) const {
        auto it = begin();
        size_t index = 0;
        while (it != end() && *it != value) {
            it++;
            ++index;
        }
        assert(it != end());
        return index;
    }

    std::string toString() const {
        std::ostringstream stream;

        bool isFirst = true;
        forEach([&](int32_t value) {
            if (isFirst) {
                isFirst = false;
            } else {
                stream << ", ";
            }
            stream << value;
        });

        return stream.str();
    }

private:
    void remove(IntSet::iterator selfIterator, IntSet::const_iterator otherIterator,
                IntSet::const_iterator otherEnd) {

        if (selfIterator == end() || otherIterator == otherEnd)
            return;

        if (*selfIterator < *otherIterator) {
            selfIterator++;
            remove(selfIterator, otherIterator, otherEnd);
        } else if (*selfIterator == *otherIterator) {
            selfIterator = erase(selfIterator);
            otherIterator++;
            remove(selfIterator, otherIterator, otherEnd);
        } else /* *selfIterator > *otherIterator */ {
            otherIterator++;
            remove(selfIterator, otherIterator, otherEnd);
        }
    }
};

} // namespace android

#endif
