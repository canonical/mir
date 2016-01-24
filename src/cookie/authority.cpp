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

#include "mir/cookie/authority.h"
#include "mir/cookie/blob.h"
#include "hmac_cookie.h"
#include "format.h"

#include <algorithm>
#include <random>
#include <memory>
#include <system_error>

#include <nettle/hmac.h>
#include <sodium/utils.h>

#include <linux/random.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>

#include <boost/throw_exception.hpp>

namespace
{
std::string const random_device_path{"/dev/random"};
std::string const urandom_device_path{"/dev/urandom"};
uint32_t const wait_seconds{30};
uint32_t const mac_byte_size{20};

size_t cookie_size_from_format(mir::cookie::Format const& format)
{
    switch (format)
    {
        case mir::cookie::Format::hmac_sha_1_8:
            return mir::cookie::default_blob_size;
        default:
            break;
    }

    return 0;
}
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

        std::generate(std::begin(buffer), std::end(buffer), [&]() { return dist(rand_dev); });
    }
    else
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to read from device: " + random_device_path +
                                                 " after: " + std::to_string(wait_seconds) + " seconds"));
    }

    return buffer;
}

mir::cookie::SecurityCheckError::SecurityCheckError() :
    runtime_error("Invalid Cookie")
{
}

class AuthorityNettle : public mir::cookie::Authority
{
public:
    AuthorityNettle(mir::cookie::Secret const& secret)
    {
        if (secret.size() < minimum_secret_size)
            BOOST_THROW_EXCEPTION(std::logic_error("Secret size " + std::to_string(secret.size()) + " is to small, require " +
                                                   std::to_string(minimum_secret_size) + " or greater."));

        hmac_sha1_set_key(&ctx, secret.size(), secret.data());
    }

    virtual ~AuthorityNettle() noexcept = default;


    std::unique_ptr<mir::cookie::Cookie> make_cookie(uint64_t const& timestamp) override
    {
        return std::make_unique<mir::cookie::HMACCookie>(timestamp, calculate_cookie(timestamp), mir::cookie::Format::hmac_sha_1_8);
    }

    std::unique_ptr<mir::cookie::Cookie> make_cookie(std::vector<uint8_t> const& raw_cookie) override
    {
        /*
        SHA_1 Format:
        1  byte  = FORMAT
        8  bytes = TIMESTAMP
        20 bytes = MAC
        */

        if (raw_cookie.size() != cookie_size_from_format(mir::cookie::Format::hmac_sha_1_8))
        {
           BOOST_THROW_EXCEPTION(mir::cookie::SecurityCheckError());
        }

        mir::cookie::Format format = static_cast<mir::cookie::Format>(raw_cookie[0]);
        if (format != mir::cookie::Format::hmac_sha_1_8)
        {
            BOOST_THROW_EXCEPTION(mir::cookie::SecurityCheckError());
        }

        uint64_t timestamp = 0;

        auto ptr = raw_cookie.data();
        ptr++;

        memcpy(&timestamp, ptr, sizeof(uint64_t));
        ptr += sizeof(timestamp);

        std::vector<uint8_t> mac(mac_byte_size);
        memcpy(mac.data(), ptr, mac.size());

        std::unique_ptr<mir::cookie::Cookie> cookie =
            std::make_unique<mir::cookie::HMACCookie>(timestamp, mac, mir::cookie::Format::hmac_sha_1_8);

        if (!verify_cookie(timestamp, cookie))
        {
            BOOST_THROW_EXCEPTION(mir::cookie::SecurityCheckError());
        }

        return cookie;
    }

private:
    std::vector<uint8_t> calculate_cookie(uint64_t const& timestamp)
    {
        std::vector<uint8_t> mac(mac_byte_size);
        hmac_sha1_update(&ctx, sizeof(timestamp), reinterpret_cast<uint8_t const*>(&timestamp));
        hmac_sha1_digest(&ctx, sizeof(mir::cookie::default_blob_size), reinterpret_cast<uint8_t*>(mac.data()));

        return mac;
    }

    bool verify_cookie(uint64_t const& timestamp, std::unique_ptr<mir::cookie::Cookie> const& cookie)
    {
        auto const calculated_cookie = make_cookie(timestamp);

        auto const this_stream  = cookie->serialize();
        auto const other_stream = calculated_cookie->serialize();

        return this_stream.size() == other_stream.size() &&
               sodium_memcmp(this_stream.data(), other_stream.data(), this_stream.size()) == 0;
    }

    struct hmac_sha1_ctx ctx;
};

size_t mir::cookie::Authority::optimal_secret_size()
{
    // Secret keys smaller than this are internally zero-extended to this size.
    // Secret keys larger than this are internally hashed to this size.
    size_t const hmac_sha1_block_size{64};
    return hmac_sha1_block_size;
}

std::unique_ptr<mir::cookie::Authority> mir::cookie::Authority::create_from(mir::cookie::Secret const& secret)
{
  return std::make_unique<AuthorityNettle>(secret);
}

std::unique_ptr<mir::cookie::Authority> mir::cookie::Authority::create_saving(mir::cookie::Secret& save_secret)
{
  save_secret = get_random_data(optimal_secret_size());
  return std::make_unique<AuthorityNettle>(save_secret);
}

std::unique_ptr<mir::cookie::Authority> mir::cookie::Authority::create()
{
  auto secret = get_random_data(optimal_secret_size());
  return std::make_unique<AuthorityNettle>(secret);
}
