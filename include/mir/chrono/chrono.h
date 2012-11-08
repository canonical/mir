/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voß <thomas.voss@canonical.com
 */


#ifndef MIR_CHRONO_CHRONO_H_
#define MIR_CHRONO_CHRONO_H_

#ifdef ANDROID
#define MIR_USING_BOOST_CHRONO
#endif // ANDROID

#ifndef MIR_USING_BOOST_CHRONO
#include <chrono>
#else // MIR_USING_BOOST_CHRONO
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

namespace mir
{
namespace std
{

using namespace ::std;

namespace chrono
{

typedef ::boost::posix_time::time_duration duration;

// Pull in typedefs
using ::boost::posix_time::microseconds;
using ::boost::posix_time::milliseconds;
using ::boost::posix_time::seconds;
using ::boost::posix_time::minutes;
using ::boost::posix_time::hours;

namespace system_clock
{
    inline boost::posix_time::ptime now()
    {
	return boost::posix_time::microsec_clock::local_time();
    }
}

// Clocks
using ::boost::chrono::high_resolution_clock;
}

}
}

#endif

#endif // MIR_CHRONO_CHRONO_H_
