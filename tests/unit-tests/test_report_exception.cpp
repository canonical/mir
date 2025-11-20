/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/report_exception.h>
#include <mir/abnormal_exit.h>
#include "boost/throw_exception.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace mir;

TEST(ReportExceptionTest, reports_abnormal_exit_to_err_stream)
{
    std::ostringstream out, err;
    std::string const message{"test-abnormal-exit"};
    try
    {
        throw AbnormalExit{message};
    }
    catch(...)
    {
        report_exception(out, err);
    }
    EXPECT_THAT(out.str(), IsEmpty());
    EXPECT_THAT(err.str(), Eq(message + "\n"));
}

TEST(ReportExceptionTest, reports_exit_with_output_to_out_stream)
{
    std::ostringstream out, err;
    std::string const message{"test-exit-with-output"};
    try
    {
        throw ExitWithOutput{message};
    }
    catch(...)
    {
        report_exception(out, err);
    }
    EXPECT_THAT(err.str(), IsEmpty());
    EXPECT_THAT(out.str(), Eq(message + "\n"));
}

TEST(ReportExceptionTest, reports_generic_exception_to_err_stream)
{
    std::ostringstream out, err;
    std::string const message{"test-abnormal-exit"};
    try
    {
        throw std::logic_error{message};
    }
    catch(...)
    {
        report_exception(out, err);
    }
    EXPECT_THAT(out.str(), IsEmpty());
    EXPECT_THAT(err.str(), HasSubstr(message));
}

TEST(ReportExceptionTest, reports_all_nested_exceptions)
{
    std::ostringstream out, err;
    std::string const message_a{"first-exception"};
    std::string const message_b{"second-exception"};
    std::string const message_c{"third-exception"};
    try
    {
        try
        {
            try
            {
                throw std::logic_error{message_a};
            }
            catch (...)
            {
                std::throw_with_nested(std::runtime_error{message_b});
            }
        }
        catch (...)
        {
            std::throw_with_nested(std::out_of_range{message_c});
        }

    }
    catch(...)
    {
        report_exception(out, err);
    }
    EXPECT_THAT(out.str(), IsEmpty());
    EXPECT_THAT(err.str(), HasSubstr(message_a));
    EXPECT_THAT(err.str(), HasSubstr(message_b));
    EXPECT_THAT(err.str(), HasSubstr(message_c));
}

TEST(ReportExceptionTest, reports_line_number_when_exception_thrown_with_boost)
{
    std::ostringstream out, err;
    int const line = __LINE__ + 3; // the exception is thrown three lines down
    try
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"hello"});
    }
    catch(...)
    {
        report_exception(out, err);
    }
    EXPECT_THAT(out.str(), IsEmpty());
    EXPECT_THAT(err.str(), HasSubstr(std::to_string(line)));
}

TEST(ReportExceptionTest, handles_strange_exception_type)
{
    std::ostringstream out, err;
    try
    {
        throw 42;
    }
    catch(...)
    {
        report_exception(out, err);
    }
    EXPECT_THAT(out.str(), IsEmpty());
    EXPECT_THAT(err.str(), Not(IsEmpty()));
}
