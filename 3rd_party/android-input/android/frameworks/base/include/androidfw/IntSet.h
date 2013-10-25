/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

#ifndef ANDROID_INTSET_H
#define ANDROID_INTSET_H

#include <assert.h>

// C++ std lib
#include <algorithm>
#include <sstream>
#include <set>

#ifdef ANDROID_INPUT_INTSET_TEST
namespace test {
#endif

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

    IntSet();
    IntSet(std::initializer_list<int32_t> list);
    virtual ~IntSet();

    IntSet operator -(const IntSet &other) const;

    IntSet operator &(const IntSet &other) const;

    template<typename Func>
    void forEach(Func func) { std::for_each(begin(), end(), func); }

    template<typename Func>
    void forEach(Func func) const { std::for_each(begin(), end(), func); }

    void remove(int32_t value) { erase(value); }
    void remove(const IntSet &values);

    size_t count() const { return size(); }

    bool isEmpty() const { return empty(); }

    bool contains(int32_t value) const;

    int32_t first() const { return *cbegin(); }

    // It's assumed that the given value does exist in the set
    size_t indexOf(int32_t value) const;

    std::string toString() const;

private:
    void remove(IntSet::iterator selfIterator, IntSet::const_iterator otherIterator,
                IntSet::const_iterator otherEnd);
};

} // namespace android

#ifdef ANDROID_INPUT_INTSET_TEST
} // namespace test
#endif

#endif
