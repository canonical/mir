#include <gtest/gtest.h>

#include <cucumber-cpp/internal/hook/Tag.hpp>

using namespace cuke::internal;

#include <boost/assign/list_of.hpp>
using namespace boost::assign;

TEST(TagTest, emptyOrExpressionMatchesNoTag) {
    OrTagExpression tagExpr("");
    EXPECT_FALSE(tagExpr.matches(list_of("x")));
    EXPECT_FALSE(tagExpr.matches(list_of("a")("b")));
}

TEST(TagTest, orExpressionsMatchTheTagSpecified) {
    OrTagExpression tagExpr("@a");
    EXPECT_TRUE(tagExpr.matches(list_of("a")));
    EXPECT_FALSE(tagExpr.matches(list_of("x")));
}

TEST(TagTest, orExpressionsMatchAnyTagSpecified) {
    OrTagExpression tagExpr("@a,@b,@c");
    EXPECT_TRUE(tagExpr.matches(list_of("a")));
    EXPECT_TRUE(tagExpr.matches(list_of("b")));
    EXPECT_TRUE(tagExpr.matches(list_of("a")("b")));
    EXPECT_TRUE(tagExpr.matches(list_of("a")("b")("c")));
    EXPECT_TRUE(tagExpr.matches(list_of("x")("a")("b")));
    EXPECT_TRUE(tagExpr.matches(list_of("a")("y")));
    EXPECT_TRUE(tagExpr.matches(list_of("x")("b")));
    EXPECT_FALSE(tagExpr.matches(list_of("x")("y")("z")));
}

TEST(TagTest, orExpressionsAllowSpaces) {
    OrTagExpression tagExpr("@a, @b,@c");
    EXPECT_TRUE(tagExpr.matches(list_of("b")));
    EXPECT_FALSE(tagExpr.matches(list_of("x")));
}


TEST(TagTest, emptyAndExpressionMatchesAnyTag) {
    AndTagExpression tagExpr("");
    EXPECT_TRUE(tagExpr.matches(list_of("x")));
    EXPECT_TRUE(tagExpr.matches(list_of("a")("b")));
}

TEST(TagTest, singleAndExpressionMatchesTheTagSpecified) {
    AndTagExpression tagExpr("\"@a\"");
    EXPECT_TRUE(tagExpr.matches(list_of("a")));
    EXPECT_FALSE(tagExpr.matches(list_of("x")));
}

TEST(TagTest, andExpressionsMatchEveryTagSpecified) {
    AndTagExpression tagExpr("\"@a\",\"@b\"");
    EXPECT_TRUE(tagExpr.matches(list_of("a")("b")));
    EXPECT_TRUE(tagExpr.matches(list_of("x")("a")("b")));
    EXPECT_FALSE(tagExpr.matches(list_of("a")));
    EXPECT_FALSE(tagExpr.matches(list_of("b")));
    EXPECT_FALSE(tagExpr.matches(list_of("a")("y")));
    EXPECT_FALSE(tagExpr.matches(list_of("x")("b")));
    EXPECT_FALSE(tagExpr.matches(list_of("x")("y")));
}

TEST(TagTest, andExpressionsAllowSpaces) {
    AndTagExpression tagExpr(" \"@a\" , \"@b\" ");
    EXPECT_TRUE(tagExpr.matches(list_of("a")("b")));
    EXPECT_FALSE(tagExpr.matches(list_of("a")));
}

TEST(TagTest, compositeTagExpressionsAreHandled) {
    AndTagExpression tagExpr("\"@a,@b\", \"@c\", \"@d,@e,@f\"");
    EXPECT_TRUE(tagExpr.matches(list_of("a")("c")("d")));
    EXPECT_FALSE(tagExpr.matches(list_of("x")("c")("f")));
}
