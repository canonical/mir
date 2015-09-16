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

using Secret = std::vector<uint8_t>;

typedef struct MirCookie
{
    uint64_t timestamp;
    uint64_t mac;
} MirCookie;

class CookieFactory
{
public:
    /**
    *   Contruction function used to create a CookieFactory. The secret size must be
    *   no less then minimum_secret_size otherwise an expection will be thrown
    *
    *   \param [in] secret  A filled in secret used to set the key for the hash function
    *   \return             A unique_ptr CookieFactory
    */
    static std::unique_ptr<CookieFactory> create_from_secret(Secret const& secret);

    /**
    *   Contruction function used to create a CookieFactory as well as a secret.
    *   The secret size must be no less then minimum_secret_size otherwise an expection will be thrown
    *
    *   \param [in]  secret_size  The size of the secret to create, must be larger then minimum_secret_size
    *   \param [out] save_secret  The secret that was created.
    *   \return                   A unique_ptr CookieFactory
    */
    static std::unique_ptr<CookieFactory> create_saving_secret(Secret& save_secret,
                                                               unsigned secret_size = 2 * minimum_secret_size);

    /**
    *   Contruction function used to create a CookieFactory and a secret which it keeps internally.
    *   The secret size must be no less then minimum_secret_size otherwise an expection will be thrown
    *
    *   \param [in]  secret_size  The size of the secret to create, must be larger then minimum_secret_size
    *   \return                   A unique_ptr CookieFactory
    */
    static std::unique_ptr<CookieFactory> create_keeping_secret(unsigned secret_size = 2 * minimum_secret_size);

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

    static unsigned const minimum_secret_size = 8;

protected:
    CookieFactory() = default;
};

}
}
#endif // MIR_COOKIE_FACTORY_H_
