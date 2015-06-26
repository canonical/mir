/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for moredetails.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_FRAMEWORK_MAIN_H
#define MIR_TEST_FRAMEWORK_MAIN_H

namespace mir_test_framework
{
/**
 * Initialize and run the mir test framework as follows:
 *
 *    ::testing::InitGoogleTest(&argc, argv);
 *    set_commandline(argc, argv);
 *    return RUN_ALL_TESTS();
 *
 * \attention If you override main() for your own purposes call this or do
 * something equivalent to run the tests.
 */
int main(int argc, char* argv[]);

/**
 * Note the commandline for use in the mir test framework. The parameter list
 * referenced by argv must remain valid during the tests.
 */
void set_commandline(int argc, char* argv[]);
}

#endif //MIR_TEST_FRAMEWORK_MAIN_H
