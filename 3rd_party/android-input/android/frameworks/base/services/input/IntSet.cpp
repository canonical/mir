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

IntSet::IntSet() {
#ifdef ANDROID_INPUT_INTSET_TEST
        ++constructionCount;
#endif
}

IntSet::IntSet(std::initializer_list<int32_t> list)
    : stdSet(list) {
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

    std::set_difference(stdSet.cbegin(), stdSet.cend(),
            other.stdSet.cbegin(), other.stdSet.cend(),
            std::inserter(result.stdSet, result.stdSet.begin()));

    return result;
}

IntSet IntSet::operator &(const IntSet &other) const {
    IntSet result;

    std::set_intersection(stdSet.cbegin(), stdSet.cend(),
            other.stdSet.cbegin(), other.stdSet.cend(),
            std::inserter(result.stdSet, result.stdSet.begin()));

    return result;
}

bool IntSet::operator ==(const IntSet &other) const {
    return stdSet == other.stdSet;
}

void IntSet::remove(const IntSet &values) {
    remove(stdSet.begin(), values.stdSet.begin(), values.stdSet.end());
}

bool IntSet::contains(int32_t value) const {
    return stdSet.find(value) != stdSet.end();
}

size_t IntSet::indexOf(int32_t value) const {
    auto it = stdSet.begin();
    size_t index = 0;
    while (it != stdSet.end() && *it != value) {
        it++;
        ++index;
    }
    assert(it != stdSet.end());
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

void IntSet::remove(std::set<int32_t>::iterator selfIterator,
                    std::set<int32_t>::const_iterator otherIterator,
                    std::set<int32_t>::const_iterator otherEnd) {

    if (selfIterator == stdSet.end() || otherIterator == otherEnd)
        return;

    if (*selfIterator < *otherIterator) {
        selfIterator++;
        remove(selfIterator, otherIterator, otherEnd);
    } else if (*selfIterator == *otherIterator) {
        selfIterator = stdSet.erase(selfIterator);
        otherIterator++;
        remove(selfIterator, otherIterator, otherEnd);
    } else /* *selfIterator > *otherIterator */ {
        otherIterator++;
        remove(selfIterator, otherIterator, otherEnd);
    }
}
