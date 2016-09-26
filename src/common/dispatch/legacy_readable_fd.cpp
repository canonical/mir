/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#define ReadableFd LegacyReadableFd

#include "mir/dispatch/readable_fd.h"

__asm__(".symver _ZN3mir8dispatch16LegacyReadableFd8dispatchEj,_ZN3mir8dispatch10ReadableFd8dispatchEj@MIR_COMMON_5.1");
__asm__(".symver _ZN3mir8dispatch16LegacyReadableFdC1ENS_2FdERKSt8functionIFvvEE,_ZN3mir8dispatch10ReadableFdC1ENS_2FdERKSt8functionIFvvEE@MIR_COMMON_5.1");
__asm__(".symver _ZN3mir8dispatch16LegacyReadableFdC2ENS_2FdERKSt8functionIFvvEE,_ZN3mir8dispatch10ReadableFdC2ENS_2FdERKSt8functionIFvvEE@MIR_COMMON_5.1");
__asm__(".symver _ZN3mir8dispatch16LegacyReadableFdD0Ev,_ZN3mir8dispatch10ReadableFdD0Ev@MIR_COMMON_5.1");
__asm__(".symver _ZN3mir8dispatch16LegacyReadableFdD1Ev,_ZN3mir8dispatch10ReadableFdD1Ev@MIR_COMMON_5.1");
__asm__(".symver _ZN3mir8dispatch16LegacyReadableFdD2Ev,_ZN3mir8dispatch10ReadableFdD2Ev@MIR_COMMON_5.1");
__asm__(".symver _ZNK3mir8dispatch16LegacyReadableFd8watch_fdEv,_ZNK3mir8dispatch10ReadableFd8watch_fdEv@MIR_COMMON_5.1");
__asm__(".symver _ZNK3mir8dispatch16LegacyReadableFd15relevant_eventsEv,_ZNK3mir8dispatch10ReadableFd15relevant_eventsEv@MIR_COMMON_5.1");

#include "./readable_fd.cpp"
