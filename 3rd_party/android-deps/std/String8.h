/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_ANDROID_UBUNTU_STRING8_H_
#define MIR_ANDROID_UBUNTU_STRING8_H_

#include <string>
#include <cstdarg>

namespace mir_input
{
typedef ::std::string String8;
inline bool isEmpty(String8 const& s) { return s.empty(); }
inline char const* c_str(String8 const& s) { return s.c_str(); }
inline String8& appendFormat(String8& ss, const char* fmt, ...)
{
    ::va_list args;
    ::va_start(args, fmt);

    int n = ::vsnprintf(NULL, 0, fmt, args);
    if (n != 0) {
        char* s = (char*) malloc(n+1);

        if (s) {
            ::va_end(args);
            ::va_start(args, fmt);
            if (::vsnprintf(s, n+1, fmt, args))
            {
                ss.append(s, s+n);
            }
            free(s);
        }
    }
    ::va_end(args);
    return ss;
}
inline void setTo(String8& s, char const* value) { s = value; }
inline char* lockBuffer(String8& s, int) { return const_cast<char*>(s.data()); }
template <typename ... Args>
inline String8 formatString8(const char* fmt, Args... args)
{
    String8 ss;
    appendFormat(ss, fmt, args...);
    return ss;
}
inline void setTo(String8& s, String8 const& value) { s = value; }
inline String8 const& emptyString8() { static String8 empty; return empty; }
}

namespace android
{
using ::mir_input::String8;
using ::mir_input::isEmpty;
using ::mir_input::c_str;
using ::mir_input::appendFormat;
using ::mir_input::setTo;
using ::mir_input::lockBuffer;
using ::mir_input::formatString8;
using ::mir_input::setTo;
using ::mir_input::emptyString8;
}

#endif /* MIR_ANDROID_UBUNTU_STRING8_H_ */
