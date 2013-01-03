#include <cucumber-cpp/internal/CukeEngineImpl.hpp>
#include <cucumber-cpp/internal/connectors/wire/WireServer.hpp>
#include <cucumber-cpp/internal/connectors/wire/WireProtocol.hpp>
#include <iostream>

using namespace cuke::internal;

void acceptWireProtocol(int port) {
    using namespace ::cuke::internal;
    CukeEngineImpl cukeEngine;
    JsonSpiritWireMessageCodec wireCodec;
    WireProtocolHandler protocolHandler(&wireCodec, &cukeEngine);
    SocketServer server(&protocolHandler);
    server.listen(port);
    server.acceptOnce();
}


int main(int argc, char **argv) {
    try {
        int port = 3902;
        if (argc > 1) {
            std::string firstArg(argv[1]);
            port = ::cuke::internal::fromString<int>(firstArg);
        }
        std::clog << "Listening on port " << port << std::endl;
        acceptWireProtocol(port);
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    return 0;
}
