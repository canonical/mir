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

#include <androidfw/IntSet.h>

#ifdef ANDROID_INPUT_INTSET_TEST
using namespace test::android;
#else
using namespace android;
#endif

IntSet::IntSet() : std::set<int32_t>() {
#ifdef ANDROID_INPUT_INTSET_TEST
        ++constructionCount;
#endif
}

IntSet::IntSet(std::initializer_list<int32_t> list)
    : std::set<int32_t>(list) {
#ifdef ANDROID_INPUT_INTSET_TEST
    ++constructionCount;
#endif
}

IntSet::~IntSet() {
#ifdef ANDROID_INPUT_INTSET_TEST
    ++destructionCount;
#endif
}

IntSet IntSet::operator -(const IntSet &other) const {
    IntSet result;

    std::set_difference(cbegin(), cend(),
            other.cbegin(), other.cend(),
            std::inserter(result, result.begin()));

    return result;
}

IntSet IntSet::operator &(const IntSet &other) const {
    IntSet result;

    std::set_intersection(cbegin(), cend(),
            other.cbegin(), other.cend(),
            std::inserter(result, result.begin()));

    return result;
}

void IntSet::remove(const IntSet &values) {
    remove(begin(), values.begin(), values.end());
}

bool IntSet::contains(int32_t value) const {
    return find(value) != end();
}

size_t IntSet::indexOf(int32_t value) const {
    auto it = begin();
    size_t index = 0;
    while (it != end() && *it != value) {
        it++;
        ++index;
    }
    assert(it != end());
    return index;
}

std::string IntSet::toString() const {
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

void IntSet::remove(IntSet::iterator selfIterator, IntSet::const_iterator otherIterator,
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
