#include <gtest/gtest.h>

#include "../utils/ContextManagerTestDouble.hpp"

using namespace std;
using namespace cuke::internal;

class ContextHandlingTest : public ::testing::Test {
public:
    ContextHandlingTest() :
        contextManager() {
    }
protected:
    ContextManagerTestDouble contextManager;

private:
    void TearDown() {
        contextManager.purgeContexts();
    }
};

struct Context1 {
    int i;
};
struct Context2 {};

TEST_F(ContextHandlingTest, contextsAreCreatedWhenNeeded) {
    ASSERT_EQ(0, contextManager.countContexts());
    ::cuke::internal::SessionContextPtr<Context1> context1;
    ASSERT_EQ(1, contextManager.countContexts());
    ::cuke::internal::SessionContextPtr<Context2> context2;
    ASSERT_EQ(2, contextManager.countContexts());
}

TEST_F(ContextHandlingTest, sameContextTypesShareTheSamePointer) {
    ::cuke::internal::SessionContextPtr<Context1> context1_a;
    ::cuke::internal::SessionContextPtr<Context1> context1_b;
    context1_a->i = 42;
    ASSERT_EQ(context1_a->i, context1_b->i);
}

TEST_F(ContextHandlingTest, theSameContextIsNotCreatedTwice) {
    ASSERT_EQ(0, contextManager.countContexts());
    ::cuke::internal::SessionContextPtr<Context1> context1_a;
    ASSERT_EQ(1, contextManager.countContexts());
    ::cuke::internal::SessionContextPtr<Context1> context1_b;
    ASSERT_EQ(1, contextManager.countContexts());
}

TEST_F(ContextHandlingTest, contextsArePurgedExplicitlyOnly) {
    ASSERT_EQ(0, contextManager.countContexts());
    ::cuke::internal::SessionContextPtr<Context1> context1_a;
    ASSERT_EQ(1, contextManager.countContexts());
    ::cuke::internal::SessionContextPtr<Context2> *context1_b =
        new ::cuke::internal::SessionContextPtr<Context2>();
    ASSERT_EQ(2, contextManager.countContexts());
    delete context1_b;
    ASSERT_EQ(2, contextManager.countContexts());
    contextManager.purgeContexts();
    ASSERT_EQ(0, contextManager.countContexts());
}

