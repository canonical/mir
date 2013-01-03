#include <CppSpec/CppSpec.h>
#include <cucumber-cpp/defs.hpp>

#include <Calculator.h>

struct CalcCtx {
    Calculator calc;
    double result;
};

GIVEN("^I have entered (\\d+) into the calculator$") {
    REGEX_PARAM(double, n);
    USING_CONTEXT(CalcCtx, context);
    context->calc.push(n);
}

WHEN("^I press add") {
    USING_CONTEXT(CalcCtx, context);
    context->result = context->calc.add();
}

WHEN("^I press divide") {
    USING_CONTEXT(CalcCtx, context);
    context->result = context->calc.divide();
}

THEN("^the result should be (.*) on the screen$") {
    REGEX_PARAM(double, expected);
    USING_CONTEXT(CalcCtx, context);
    specify(context->result, should.equal(expected));
}
