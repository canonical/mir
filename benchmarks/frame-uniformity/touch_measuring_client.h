/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef TOUCH_MEASURING_CLIENT_H_
#define TOUCH_MEASURING_CLIENT_H_

#include "touch_samples.h"

#include "mir/test/barrier.h"

#include <chrono>
#include <memory>

class TouchMeasuringClient
{
public:
    TouchMeasuringClient(mir::test::Barrier& client_ready,
        std::chrono::high_resolution_clock::duration const& touch_duration);
    
    void run(std::string const& connect_string);
    
    std::shared_ptr<TouchSamples> results();

private:
    mir::test::Barrier& client_ready;
    
    std::chrono::high_resolution_clock::duration const touch_duration;
    
    std::shared_ptr<TouchSamples> results_;
};

#endif // TOUCH_MEASURING_CLIENT_H_
