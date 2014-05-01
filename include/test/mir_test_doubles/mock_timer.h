#ifndef MIR_TEST_DOUBLES_MOCK_TIMER_H_
#define MIR_TEST_DOUBLES_MOCK_TIMER_H_

#include "mir/time/timer.h"

namespace mir
{
namespace test
{
namespace doubles
{

class MockAlarm : public mir::time::Alarm
{
public:
    MockAlarm(std::function<void(void)> callback)
        : callback{callback}
    {
        using namespace testing;
        ON_CALL(*this, cancel()).WillByDefault(Return(true));
        ON_CALL(*this, state()).WillByDefault(Return(Alarm::Pending));
        ON_CALL(*this, reschedule_in(_)).WillByDefault(Return(true));
    }
    virtual ~MockAlarm() = default;

    MOCK_METHOD0(cancel, bool(void));
    MOCK_CONST_METHOD0(state, Alarm::State(void));
    MOCK_METHOD1(reschedule_in, bool(std::chrono::milliseconds));
    MOCK_METHOD1(reschedule_for, bool(mir::time::Timestamp));

    void trigger()
    {
        callback();
    }
private:
    std::function<void(void)> const callback;
};

class MockTimer : public mir::time::Timer
{
public:
    MockTimer()
    {
        using namespace testing;
        ON_CALL(*this, notify_in(_,_))
            .WillByDefault(WithArgs<1>(Invoke(create_alarm_for_callback)));
        ON_CALL(*this, notify_at(_,_))
            .WillByDefault(WithArgs<1>(Invoke(create_alarm_for_callback)));
    }

    MOCK_METHOD2(notify_in, std::unique_ptr<mir::time::Alarm>(std::chrono::milliseconds, std::function<void(void)>));
    MOCK_METHOD2(notify_at, std::unique_ptr<mir::time::Alarm>(mir::time::Timestamp, std::function<void(void)>));

    static std::unique_ptr<mir::time::Alarm> create_alarm_for_callback(std::function<void(void)> callback)
    {
        return std::unique_ptr<mir::time::Alarm>{new testing::NiceMock<MockAlarm>{callback}};
    }
};

}
}
}

#endif // MIR_TEST_DOUBLES_MOCK_TIMER_H_
