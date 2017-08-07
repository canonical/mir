/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "hmac_cookie.h"

#include <string.h>

mir::cookie::HMACCookie::HMACCookie(uint64_t const& timestamp,
                                    std::vector<uint8_t> const& mac,
                                    mir::cookie::Format const& format) :
    timestamp_(timestamp),
    mac_(mac),
    format_(format)
{
}

uint64_t mir::cookie::HMACCookie::timestamp() const
{
    return timestamp_;
}

std::vector<uint8_t> mir::cookie::HMACCookie::serialize() const
{
    std::vector<uint8_t> serialized_cookie(sizeof(format_) + sizeof(timestamp_) + mac_.size());

    auto cookie_ptr = serialized_cookie.data();

    cookie_ptr[0] = static_cast<uint8_t>(format_);
    cookie_ptr += sizeof(format_);

    auto timestamp_ptr = reinterpret_cast<uint8_t const*>(&timestamp_);
    memcpy(cookie_ptr, timestamp_ptr, sizeof(timestamp_));
    cookie_ptr += sizeof(timestamp_);

    memcpy(cookie_ptr, mac_.data(), mac_.size());
    
    return serialized_cookie;
}
