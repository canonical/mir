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

#ifndef MIR_COOKIE_FACTORY_H_
#define MIR_COOKIE_FACTORY_H_

#include <vector>
#include <memory>
#include "mir_toolkit/common.h"

namespace mir
{
namespace cookie
{
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
    *   Contruction function used to create a CookieFactory. The secret size must be
    *   greater then minimum_secret_size otherwise an expection will be thrown
    *
    *   \param [in] secret  A filled in secret used to set the key for the hash function
    *   \return             A unique_ptr CookieFactory
    */
    static std::unique_ptr<CookieFactory> create(std::vector<uint8_t> const& secret);

    /**
    *   Contruction function used to create a CookieFactory as well as a secret.
    *   The secret size must be greater then minimum_secret_size otherwise an expection will be thrown
    *
    *   \param [in]  secret_size  The size of the secret to create, must be larger then minimum_secret_size
    *   \param [out] save_secret  The secret that was created.
    *   \return                   A unique_ptr CookieFactory
    */
    static std::unique_ptr<CookieFactory> create(unsigned secret_size, std::vector<uint8_t>& save_secret);

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

protected:
    CookieFactory() = default;

    static unsigned const minimum_secret_size;
};

/**
*   Checks entropy exists on the system, then fills in a vector with random numbers
*   up to size.
*
*   \param [in] size  The number of random numbers to generate.
*   \return           A filled in vector with random numbers up to size
*/
std::vector<uint8_t> get_random_data(unsigned size);

}
}
#endif // MIR_COOKIE_FACTORY_H_
