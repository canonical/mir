/*
 * Copyright © 2015-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *              Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_COOKIE_AUTHORITY_H_
#define MIR_COOKIE_AUTHORITY_H_

#include <memory>
#include <stdexcept>
#include <vector>

#include "mir/cookie/cookie.h"

namespace mir
{
namespace cookie
{
using Secret = std::vector<uint8_t>;

struct SecurityCheckError : std::runtime_error
{
    SecurityCheckError();
};

/**
 * \brief A source of moderately-difficult-to-spoof cookies.
 *
 * The primary motivation for this is to provide event timestamps that clients find it difficult to spoof.
 * This is useful for focus grant and similar operations where shell behaviour should be dependent on
 * the timestamp of the client event that caused the request.
 *
 * Some spoofing protection is desirable; experience with X clients shows that they will go to some effort
 * to attempt to bypass focus stealing prevention.
 *
 */
class Authority
{
public:
    /**
     * Optimal size for the provided Secret.
     *
     * This is the maximum useful size of the secret key. Keys of greater size
     * will be reduced to this size internally, and keys of smaller size may be
     * internally extended to this size.
     */
    static size_t optimal_secret_size();

    /**
    *   Construction function used to create an Authority. The secret size must be
    *   no less then minimum_secret_size otherwise an exception will be thrown
    *
    *   \param [in] secret  A secret used to set the key for the hash function
    *   \return             An Authority
    */
    static std::unique_ptr<Authority> create_from(Secret const& secret);

    /**
    *   Construction function used to create an Authority as well as a secret.
    *
    *   \param [out] save_secret  The secret that was created.
    *   \return                   An Authority
    */
    static std::unique_ptr<Authority> create_saving(Secret& save_secret);

    /**
    *   Construction function used to create an Authority and a secret which it keeps internally.
    *
    *   \return                   An Authority
    */
    static std::unique_ptr<Authority> create();

    Authority(Authority const& authority) = delete;
    Authority& operator=(Authority const& authority) = delete;
    virtual ~Authority() noexcept = default;

    /**
    * Creates a cookie from a timestamp.
    *
    * \param [in] timestamp A timestamp
    * \return               A cookie instance
    */
    virtual std::unique_ptr<Cookie> make_cookie(uint64_t const& timestamp) = 0;

    /**
    * Creates a cookie from a serialized representation
    *
    * \param [in] blob A blob of bytes representing a serialized cookie
    * \return          A cookie instance
    */
    virtual std::unique_ptr<Cookie> make_cookie(std::vector<uint8_t> const& raw_cookie) = 0;

    /**
    * Absolute minimum size of secret key the Authority will accept.
    *
    * Code should be using optimum_secret_size(); this minimum size is provided
    * as a user convenience to guard against catastrophically bad initialisation.
    */
    static unsigned const minimum_secret_size = 8;
protected:
    Authority() = default;
};

}
}
#endif // MIR_COOKIE_COOKIE_AUTHORITY_H_
