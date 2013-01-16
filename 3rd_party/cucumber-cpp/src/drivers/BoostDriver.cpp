#include <cucumber-cpp/internal/drivers/BoostDriver.hpp>

#include <sstream>

#include <boost/bind.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log_formatter.hpp>

using namespace ::boost::unit_test;
using ::boost::execution_exception;

namespace cuke {
namespace internal {


namespace {

bool boost_test_init() {
    return true;
}

static CukeBoostLogInterceptor *logInterceptor = 0;

}


class CukeBoostLogInterceptor : public ::boost::unit_test::unit_test_log_formatter {
public:
    const InvokeResult getResult() const;
    void reset();

    // Formatter
    void log_start( std::ostream&, counter_t test_cases_amount) {};
    void log_finish( std::ostream&) {};
    void log_build_info( std::ostream&) {};

    void test_unit_start( std::ostream&, test_unit const& tu) {};
    void test_unit_finish( std::ostream&, test_unit const& tu, unsigned long elapsed) {};
    void test_unit_skipped( std::ostream&, test_unit const& tu) {};

    void log_exception( std::ostream&, log_checkpoint_data const&, execution_exception const& ex) {};

    void log_entry_start( std::ostream&, log_entry_data const&, log_entry_types let) {};
    void log_entry_value( std::ostream&, const_string value);
    void log_entry_value( std::ostream&, lazy_ostream const& value) {};
    void log_entry_finish( std::ostream&) {};

    void log_exception(std::ostream&, const boost::unit_test::log_checkpoint_data&, boost::unit_test::const_string) {};

private:
    std::stringstream description;
};

void CukeBoostLogInterceptor::reset() {
    description.str("");
}

/*
 * Threshold level set to log_all_errors, so we should be fine logging everything
 */
void CukeBoostLogInterceptor::log_entry_value( std::ostream&, const_string value) {
    description << value;
};

const InvokeResult CukeBoostLogInterceptor::getResult() const {
    std::string d = description.str();
    if (d.empty()) {
        return InvokeResult::success();
    } else {
        return InvokeResult::failure(description.str());
    }
}

const InvokeResult BoostStep::invokeStepBody() {
    initBoostTest();
    logInterceptor->reset();
    runWithMasterSuite();
    return logInterceptor->getResult();
}

void BoostStep::initBoostTest() {
    if (!framework::is_initialized()) {
        int argc = 2;
        char *argv[] = { (char *) "", (char *) "" };
        framework::init(&boost_test_init, argc, argv);
        logInterceptor = new CukeBoostLogInterceptor;
        ::boost::unit_test::unit_test_log.set_formatter(logInterceptor);
        ::boost::unit_test::unit_test_log.set_threshold_level(log_all_errors);
    }
}

void BoostStep::runWithMasterSuite() {
    using namespace ::boost::unit_test;
    test_case *tc = BOOST_TEST_CASE(boost::bind(&BoostStep::body, this));
    framework::master_test_suite().add(tc);
    framework::run(tc, false);
    framework::master_test_suite().remove(tc->p_id);
}

}
}
