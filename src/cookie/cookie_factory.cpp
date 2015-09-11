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

#include <algorithm>
#include <random>
#include <system_error>

#include <nettle/hmac.h>
#include <linux/random.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#include <boost/throw_exception.hpp>

namespace
{
std::string const random_device_path{"/dev/random"};
std::string const urandom_device_path{"/dev/urandom"};
int const wait_seconds{30};
unsigned const min_secret_size{8};
}

std::vector<uint8_t> mir::get_random_data(unsigned size)
{
    std::vector<uint8_t> buffer(size);
    int random_fd;
    int retval;
    fd_set rfds;

    struct timeval tv;
    tv.tv_sec  = wait_seconds;
    tv.tv_usec = 0;

    if ((random_fd = open(random_device_path.c_str(), O_RDONLY)) == -1)
    {
        int error = errno;
        BOOST_THROW_EXCEPTION(std::system_error(error, std::system_category(),
                                                "open failed on device " + random_device_path));
    }

    FD_ZERO(&rfds);
    FD_SET(random_fd, &rfds);

    /* We want to block until *some* entropy exists on boot, then use urandom once we have some */
    retval = select(random_fd + 1, &rfds, NULL, NULL, &tv);

    /* We are done with /dev/random at this point, and it is either an error or ready to be read */
    if (close(random_fd) == -1)
    {
        int error = errno;
        BOOST_THROW_EXCEPTION(std::system_error(error, std::system_category(),
                                                "close failed on device " + random_device_path));
    }

    if (retval == -1)
    {
        int error = errno;
        BOOST_THROW_EXCEPTION(std::system_error(error, std::system_category(),
                                                "select failed on file descriptor " + std::to_string(random_fd) +
                                                " from device " + random_device_path));
    }
    else if (retval && FD_ISSET(random_fd, &rfds))
    {
        std::uniform_int_distribution<uint8_t> dist;
        std::random_device rand_dev(urandom_device_path);

        std::generate(std::begin(buffer), std::end(buffer), [&]() {
            return dist(rand_dev);
        });
    }
    else
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to read from device: " + random_device_path +
                                                 " after: " + std::to_string(wait_seconds) + " seconds"));
    }

    return buffer;
}

class mir::CookieFactory::CookieImpl
{
public:
    CookieImpl(std::vector<uint8_t> const& secret)
    {
        if (secret.size() < min_secret_size)
            BOOST_THROW_EXCEPTION(std::runtime_error("Secret size " + std::to_string(secret.size()) + " is to small, require " +
                                                     std::to_string(min_secret_size) + " or greater."));

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
    MirCookie cookie { timestamp, 0 };
    impl->calculate_mac(cookie);
    return cookie;
}

bool mir::CookieFactory::attest_timestamp(MirCookie const& cookie)
{
    return impl->verify_mac(cookie);
}
