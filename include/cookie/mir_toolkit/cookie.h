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

#ifndef COOKIE_FACTORY_H_
#define COOKIE_FACTORY_H_

/* This is C code. Not C++. */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MirCookie
{
    uint64_t timestamp;
    uint64_t mac;
} MirCookie;

#ifdef __cplusplus
}
#endif
/**@}*/

#endif // COOKIE_FACTORY_H_
