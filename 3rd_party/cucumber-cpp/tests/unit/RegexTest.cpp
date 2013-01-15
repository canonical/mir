#include <gtest/gtest.h>

#include <boost/assign/list_of.hpp>
using namespace boost::assign;

#include <cucumber-cpp/internal/utils/Regex.hpp>
using namespace cuke::internal;

#include <boost/shared_ptr.hpp>
using boost::shared_ptr;


TEST(RegexTest, matchesSimpleRegex) {
    Regex exact("^cde$");

    shared_ptr<RegexMatch> match(exact.find("cde"));
    EXPECT_TRUE(match->matches());
    EXPECT_TRUE(match->getSubmatches().empty());

    match = shared_ptr<RegexMatch>(exact.find("abcdefg"));
    EXPECT_FALSE(match->matches());
    EXPECT_TRUE(match->getSubmatches().empty());
}

TEST(RegexTest, matchesRegexWithoutSubmatches) {
    Regex variable("x\\d+x");

    shared_ptr<RegexMatch> match(variable.find("xxxx123xxx"));
    EXPECT_TRUE(match->matches());

    match = shared_ptr<RegexMatch>(variable.find("xxx"));
    EXPECT_FALSE(match->matches());
}

TEST(RegexTest, matchesRegexWithSubmatches) {
    Regex sum("^(\\d+)\\+\\d+=(\\d+)$");

    shared_ptr<RegexMatch> match(sum.find("1+2=3 "));
    EXPECT_FALSE(match->matches());
    EXPECT_TRUE(match->getSubmatches().empty());

    match = shared_ptr<RegexMatch>(sum.find("42+27=69"));
    EXPECT_TRUE(match->matches());
    ASSERT_EQ(2, match->getSubmatches().size());
    EXPECT_EQ("42", match->getSubmatches()[0].value);
    EXPECT_EQ("69", match->getSubmatches()[1].value);
}

TEST(RegexTest, findAllDoesNotMatchIfNoTokens) {
    Regex sum("([^,]+)(?:,|$)");
    shared_ptr<RegexMatch> match(sum.findAll(""));

    EXPECT_FALSE(match->matches());
    EXPECT_EQ(0, match->getSubmatches().size());
}

TEST(RegexTest, findAllExtractsTheFirstGroupOfEveryToken) {
    Regex sum("([^,]+)(?:,|$)");
    shared_ptr<RegexMatch> match(sum.findAll("a,b,cc"));

    EXPECT_TRUE(match->matches());
    EXPECT_EQ(3, match->getSubmatches().size());
    //EXPECT_THAT(match.getSubmatches(), ElementsAre("a", "b", "cc"));
}

/*
TEST(RegexTest, findAllHasToMatchTheEntireInput) {
    Regex sum("([^,]+)(?:,|$)");
    shared_ptr<RegexMatch> match(sum.findAll("1 a,b,cc"));

    EXPECT_FALSE(match->matches());
    EXPECT_EQ(0, match->getSubmatches().size());
}
*/
