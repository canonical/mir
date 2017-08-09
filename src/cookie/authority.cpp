/*
 * Copyright © 2015-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
#include "const_memcmp.h"
#include "format.h"

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
#include <string.h>

#include <boost/throw_exception.hpp>

namespace
{
uint32_t const mac_byte_size{32};

size_t cookie_size_from_format(mir::cookie::Format const& format)
{
    switch (format)
    {
        case mir::cookie::Format::hmac_sha_256:
            return mir::cookie::default_blob_size;
        default:
            break;
    }

    return 0;
}
}

static mir::cookie::Secret get_random_data(unsigned size)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
        BOOST_THROW_EXCEPTION(std::system_error(errno, std::system_category(),
                                                "open failed on urandom"));

    mir::cookie::Secret buffer(size);
    unsigned got = read(fd, buffer.data(), size);
    int error = errno;
    close(fd);

    if (got != size)
        BOOST_THROW_EXCEPTION(std::system_error(error, std::system_category(),
                                                "read failed on urandom"));

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

        hmac_sha256_set_key(&ctx, secret.size(), secret.data());
    }

    virtual ~AuthorityNettle() noexcept = default;


    std::unique_ptr<mir::cookie::Cookie> make_cookie(uint64_t const& timestamp) override
    {
        return std::make_unique<mir::cookie::HMACCookie>(timestamp, calculate_cookie(timestamp), mir::cookie::Format::hmac_sha_256);
    }

    std::unique_ptr<mir::cookie::Cookie> make_cookie(std::vector<uint8_t> const& raw_cookie) override
    {
        /*
        SHA_256 Format:
        1  byte  = FORMAT
        8  bytes = TIMESTAMP
        32 bytes = MAC
        */

        if (raw_cookie.size() != cookie_size_from_format(mir::cookie::Format::hmac_sha_256))
        {
           BOOST_THROW_EXCEPTION(mir::cookie::SecurityCheckError());
        }

        mir::cookie::Format format = static_cast<mir::cookie::Format>(raw_cookie[0]);
        if (format != mir::cookie::Format::hmac_sha_256)
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
            std::make_unique<mir::cookie::HMACCookie>(timestamp, mac, mir::cookie::Format::hmac_sha_256);

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
        hmac_sha256_update(&ctx, sizeof(timestamp), reinterpret_cast<uint8_t const*>(&timestamp));
        hmac_sha256_digest(&ctx, mac.size(), mac.data());

        return mac;
    }

    bool verify_cookie(uint64_t const& timestamp, std::unique_ptr<mir::cookie::Cookie> const& cookie)
    {
        auto const calculated_cookie = make_cookie(timestamp);

        auto const this_stream  = cookie->serialize();
        auto const other_stream = calculated_cookie->serialize();

        return this_stream.size() == other_stream.size() &&
               mir::cookie::const_memcmp(this_stream.data(), other_stream.data(), this_stream.size()) == 0;
    }

    struct hmac_sha256_ctx ctx;
};

size_t mir::cookie::Authority::optimal_secret_size()
{
    // Secret keys smaller than this are internally zero-extended to this size.
    // Secret keys larger than this are internally hashed to this size.
    size_t const hmac_sha256_block_size{64};
    return hmac_sha256_block_size;
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
