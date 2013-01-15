#include <cucumber-cpp/internal/connectors/wire/WireServer.hpp>

#include <gmock/gmock.h>

#include <boost/thread.hpp>
#include <boost/timer.hpp>

#include <sstream>

using namespace cuke::internal;
using namespace boost::posix_time;
using namespace boost::asio::ip;
using namespace std;
using namespace testing;
using boost::bind;
using boost::thread;

static const time_duration THREAD_TEST_TIMEOUT = milliseconds(4000);

MATCHER(IsConnected, string(negation ? "is not" : "is") +
        " connected") { return arg.good(); }

MATCHER(HasTerminated, "") {
    return !arg.joinable();
}

MATCHER(EventuallyTerminates, "") {
    thread *t = const_cast<thread *>(arg);
    return t->timed_join(THREAD_TEST_TIMEOUT);
}

MATCHER_P(EventuallyReceives, value, "") {
    tcp::iostream *stream = const_cast<tcp::iostream *>(&arg);
    std::string output;
// FIXME It should not block
    (*stream) >> output;
//    boost::timer timer;
//    double timeout = THREAD_TEST_TIMEOUT.total_milliseconds() / 1000.0;
//    while (timer.elapsed() < timeout) {
//        if (stream->rdbuf()->available() > 0) { // it is zero even if it doesn't block!
//            (*stream) >> output;
//            break;
//        }
//        boost::this_thread::yield();
//    }
    return (output == value);
}

class MockProtocolHandler : public ProtocolHandler {
public:
    MOCK_CONST_METHOD1(handle, string(const string &request));
};

class SocketServerTest : public Test {

protected:
    static const unsigned short PORT = 54321;
    StrictMock<MockProtocolHandler> protocolHandler;
    SocketServer *server;
    thread *serverThread;

    virtual void SetUp() {
        server = new SocketServer(&protocolHandler);
        server->listen(PORT);
        serverThread = new thread(bind(&SocketServer::acceptOnce, server));
    }

    virtual void TearDown() {
        if (serverThread) {
            serverThread->timed_join(THREAD_TEST_TIMEOUT);
            delete serverThread;
        }
        if (server) {
            delete server;
        }
    }
};


TEST_F(SocketServerTest, exitsOnFirstConnectionClosed) {
    // given
    tcp::iostream client(tcp::endpoint(tcp::v4(), PORT));
    ASSERT_THAT(client, IsConnected());

    // when
    client.close();

    // then
    EXPECT_THAT(serverThread, EventuallyTerminates());
}

TEST_F(SocketServerTest, moreThanOneClientCanConnect) {
    // given
    tcp::iostream client1(tcp::endpoint(tcp::v4(), PORT));
    ASSERT_THAT(client1, IsConnected());

    // when
    tcp::iostream client2(tcp::endpoint(tcp::v4(), PORT));

    //then
    ASSERT_THAT(client2, IsConnected());
}

TEST_F(SocketServerTest, receiveAndSendsSingleLineMassages) {
    {
        InSequence s;
        EXPECT_CALL(protocolHandler, handle("12")).WillRepeatedly(Return("A"));
        EXPECT_CALL(protocolHandler, handle("3")).WillRepeatedly(Return("B"));
        EXPECT_CALL(protocolHandler, handle("4")).WillRepeatedly(Return("C"));
    }

    // given
    tcp::iostream client(tcp::endpoint(tcp::v4(), PORT));
    ASSERT_THAT(client, IsConnected());

    // when
    client << "1" << flush << "2" << endl << flush;
    client << "3" << endl << "4" << endl << flush;

    // then
    EXPECT_THAT(client, EventuallyReceives("A"));
    EXPECT_THAT(client, EventuallyReceives("B"));
    EXPECT_THAT(client, EventuallyReceives("C"));
}
