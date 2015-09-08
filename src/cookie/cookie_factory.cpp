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

#include "mir/cookie_factory.h"

#include <nettle/hmac.h>

class mir::CookieFactory::CookieImpl
{
public:
    CookieImpl(std::vector<uint8_t> const& secret)
    {
        hmac_sha1_set_key(&ctx, secret.size(), secret.data());
    }

    void calculate_mac(MirCookie& cookie)
    {
        hmac_sha1_update(&ctx, sizeof(cookie.timestamp), reinterpret_cast<uint8_t*>(&cookie.timestamp));
        hmac_sha1_digest(&ctx, sizeof(cookie.mac), reinterpret_cast<uint8_t*>(&cookie.mac));
    }

    bool verify_mac(MirCookie const& cookie)
    {
        decltype(cookie.mac) calculated_mac;
        uint8_t* message = reinterpret_cast<uint8_t*>(const_cast<decltype(cookie.timestamp)*>(&cookie.timestamp));
        hmac_sha1_update(&ctx, sizeof(cookie.timestamp), message);
        hmac_sha1_digest(&ctx, sizeof(calculated_mac), reinterpret_cast<uint8_t*>(&calculated_mac));

        return calculated_mac == cookie.mac;
    }

private:
    struct hmac_sha1_ctx ctx;
};

mir::CookieFactory::CookieFactory(std::vector<uint8_t> const& secret)
    : impl(std::make_unique<CookieImpl>(secret))
{
}

mir::CookieFactory::~CookieFactory() noexcept
{
}

MirCookie mir::CookieFactory::timestamp_to_cookie(uint64_t const& timestamp)
{
    MirCookie cookie { timestamp, 0};
    impl->calculate_mac(cookie);
    return cookie;
}

bool mir::CookieFactory::attest_timestamp(MirCookie const& cookie)
{
    return impl->verify_mac(cookie);
}
