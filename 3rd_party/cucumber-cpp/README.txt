Cucumber-Cpp allows Cucumber to support step definitions written in C++


Cucumber-Cpp Website: http://github.com/cucumber/cucumber-cpp
Cucumber-Cpp Documentation: https://github.com/cucumber/cucumber-cpp/wiki/
Cucumber Website: http://cukes.info/


The current library relies on a few libraries:

 * Boost 1.40 or later - http://www.boost.org/
   Required libraries: thread, system, regex, and date_time
   Optional library for Boost Test driver: test
 * GTest 1.4 or later - http://code.google.com/p/googletest/
   Optional for the GTest driver
 * CppSpec development branch - https://github.com/tpuronen/cppspec
   Optional for the CppSpec driver
 * GMock 1.6 or later - http://code.google.com/p/googlemock/
   Optional for the internal test suite

This header-only library is included in the source code:

 * JSON Spirit - http://www.codeproject.com/KB/recipes/JSON_Spirit.aspx

It might work with earlier versions of the libraries, but it was not
tested with them.


Building tests and sample application:

cmake -E make_directory build
cmake -E chdir build cmake -DCUKE_ENABLE_EXAMPLES=on ..
cmake --build build
cmake --build build --target test

Running the example on Unix:

build/examples/Calc/CalculatorSteps >/dev/null &
cucumber examples/Calc/CalcFeatures

Running the example on Windows (NMake):

start build\examples\Calc\CalculatorSteps.exe
cucumber examples\Calc\CalcFeatures
