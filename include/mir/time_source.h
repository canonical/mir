/*
 * Copyright © 2012 Canonical Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Canonical Ltd. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Canonical Ltd. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * CANONICAL, LTD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL CANONICAL, LTD. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef TIME_SOURCE_H_
#define TIME_SOURCE_H_

#include <boost/units/make_scaled_unit.hpp>
#include <boost/units/systems/si/io.hpp>
#include <boost/units/systems/si/time.hpp>

#include <cstdint>

namespace mir
{

namespace units = boost::units;
namespace si = units::si;

typedef units::make_scaled_unit<
    si::time,
    units::scale<10, units::static_rational<-6> > >::type TimestampUnit;
typedef units::quantity<TimestampUnit, uint64_t> Timestamp;

class TimeSource {
 public:
    virtual ~TimeSource() {}

    virtual Timestamp Sample() const = 0;
    
 protected:    
    TimeSource() = default;
};

}

#endif // TIME_SOURCE_H_
