#include <cucumber-cpp/internal/connectors/wire/WireServer.hpp>

namespace cuke {
namespace internal {

SocketServer::SocketServer(const ProtocolHandler *protocolHandler) :
    ios(),
    acceptor(ios),
    protocolHandler(protocolHandler) {
}

void SocketServer::listen(const port_type port) {
    tcp::endpoint endpoint(tcp::v4(), port);
    acceptor.open(endpoint.protocol());
    acceptor.set_option(tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen(1);
}

void SocketServer::acceptOnce() {
    tcp::iostream stream;
    acceptor.accept(*stream.rdbuf());
    processStream(stream);
}

void SocketServer::processStream(tcp::iostream &stream) {
    std::string request;
    while (getline(stream, request)) {
        stream << protocolHandler->handle(request) << std::endl << std::flush;
    }
}

}
}
