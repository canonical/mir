/*
 * Copyright © 2015 Canonical Ltd.
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
 */

#ifndef MIR_COOKIE_COOKIE_FACTORY_H_
#define MIR_COOKIE_COOKIE_FACTORY_H_

#include "mir_toolkit/cookie.h"

#include <vector>
#include <memory>

namespace mir
{
namespace cookie
{
using Secret = std::vector<uint8_t>;

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
class CookieFactory
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
    *   Construction function used to create a CookieFactory. The secret size must be
    *   no less then minimum_secret_size otherwise an exception will be thrown
    *
    *   \param [in] secret  A filled in secret used to set the key for the hash function
    *   \return             A unique_ptr CookieFactory
    */
    static std::unique_ptr<CookieFactory> create_from_secret(Secret const& secret);

    /**
    *   Construction function used to create a CookieFactory as well as a secret.
    *
    *   \param [out] save_secret  The secret that was created.
    *   \return                   A unique_ptr CookieFactory
    */
    static std::unique_ptr<CookieFactory> create_saving_secret(Secret& save_secret);

    /**
    *   Construction function used to create a CookieFactory and a secret which it keeps internally.
    *
    *   \return                   A unique_ptr CookieFactory
    */
    static std::unique_ptr<CookieFactory> create_keeping_secret();

    CookieFactory(CookieFactory const& factory) = delete;
    CookieFactory& operator=(CookieFactory const& factory) = delete;
    virtual ~CookieFactory() noexcept = default;

    /**
    *   Turns a timestamp into a MAC and returns a MirCookie.
    *
    *   \param [in] timestamp The timestamp
    *   \return               MirCookie with the stored MAC and timestamp
    */
    virtual MirCookie timestamp_to_cookie(uint64_t const& timestamp) = 0;

    /**
    *   Checks that a MirCookie is a valid MirCookie.
    *
    *   \param [in] cookie  A created MirCookie
    *   \return             True when the MirCookie is valid, False when the MirCookie is not valid
    */
    virtual bool attest_timestamp(MirCookie const& cookie) = 0;

    /**
     * Absolute minimum size of secret key the CookieFactory will accept.
     *
     * Code should be using optimum_secret_size(); this minimum size is provided
     * as a user convenience to guard against catastrophically bad initialisation.
     */
    static unsigned const minimum_secret_size = 8;
protected:
    CookieFactory() = default;
};

}
}
#endif // MIR_COOKIE_COOKIE_FACTORY_H_
