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
#include <memory>
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
}

static mir::cookie::Secret get_random_data(unsigned size)
{
    mir::cookie::Secret buffer(size);
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

class CookieFactoryNettle : public mir::cookie::CookieFactory
{
public:
    CookieFactoryNettle(mir::cookie::Secret const& secret)
    {
        if (secret.size() < minimum_secret_size)
            BOOST_THROW_EXCEPTION(std::logic_error("Secret size " + std::to_string(secret.size()) + " is to small, require " +
                                                   std::to_string(minimum_secret_size) + " or greater."));

        hmac_sha1_set_key(&ctx, secret.size(), secret.data());
    }

    virtual ~CookieFactoryNettle() noexcept = default;

    MirCookie timestamp_to_cookie(uint64_t const& timestamp) override
    {
        MirCookie cookie { timestamp, 0 };
        calculate_mac(cookie);
        return cookie;
    }

    bool attest_timestamp(MirCookie const& cookie) override
    {
        return verify_mac(cookie);
    }

private:
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

    struct hmac_sha1_ctx ctx;
};

size_t mir::cookie::CookieFactory::optimal_secret_size()
{
    // Secret keys smaller than this are internally zero-extended to this size.
    // Secret keys larger than this are internally hashed to this size.
    size_t const hmac_sha1_block_size{64};
    return hmac_sha1_block_size;
}

std::unique_ptr<mir::cookie::CookieFactory> mir::cookie::CookieFactory::create_from_secret(mir::cookie::Secret const& secret)
{
  return std::make_unique<CookieFactoryNettle>(secret);
}

std::unique_ptr<mir::cookie::CookieFactory> mir::cookie::CookieFactory::create_saving_secret(mir::cookie::Secret& save_secret)
{
  save_secret = get_random_data(optimal_secret_size());
  return std::make_unique<CookieFactoryNettle>(save_secret);
}

std::unique_ptr<mir::cookie::CookieFactory> mir::cookie::CookieFactory::create_keeping_secret()
{
  auto secret = get_random_data(optimal_secret_size());
  return std::make_unique<CookieFactoryNettle>(secret);
}
