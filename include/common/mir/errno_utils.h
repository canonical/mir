/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_ERRNO_UTILS_H_
#define MIR_ERRNO_UTILS_H_

#include <cerrno>
#include <cstring>
#include <string>

namespace mir
{
    namespace detail {

        // POSIX strerror_r returns int, GNU returns char*.
        // This overload set normalizes both to std::string.
        inline std::string strerror_r_to_string(int rc, const char* buf) {
            // POSIX: rc == 0 on success. On failure, buf may contain something,
            // but still provide a deterministic message.
            if (rc == 0 && buf && *buf) return std::string(buf);
            return "Unknown error (" + std::to_string(rc) + ")";
        }

        inline std::string strerror_r_to_string(const char* msg, const char* /*buf*/) {
            // GNU: returns pointer to message (may or may not be buf).
            if (msg && *msg) return std::string(msg);
            return "Unknown error";
        }

    } // namespace detail


    // Thread-safe errno to string conversion.
    inline std::string errno_to_string(int err) {
        thread_local char buf[256] = {};

        // Call strerror_r and normalize both GNU/POSIX return types.
        auto ret = ::strerror_r(err, buf, sizeof(buf));
        return detail::strerror_r_to_string(ret, buf);
    }

    inline const char* errno_to_cstr(int err) noexcept {
        thread_local char buf[256] = {};

        // Handle GNU vs POSIX strerror_r
        auto ret = ::strerror_r(err, buf, sizeof(buf));

        // POSIX strerror_r returns int, GNU returns char*.
        if constexpr (std::is_same_v<decltype(ret), int>) {
            return buf;
        } else {
            return ret ? ret : "Unknown error";
        }
    }

}

#endif /* MIR_ERRNO_UTILS_H_ */

