#include "listener.h"

// Accepts incoming connections and launches the sessions
Listener::Listener(net::io_context &ioc, tcp::endpoint endpoint,
                   std::shared_ptr<ServerState> state)
    : _ioc(ioc), _acceptor(ioc), _state(state) {
  beast::error_code ec;

  // Open the acceptor
  _acceptor.open(endpoint.protocol(), ec);
  if (ec) {
    fail(ec, "open", true);
    return;
  }

  // Allow address reuse
  _acceptor.set_option(net::socket_base::reuse_address(true), ec);
  if (ec) {
    fail(ec, "set_option");
    return;
  }

  // Bind to the server address
  _acceptor.bind(endpoint, ec);
  if (ec) {
    fail(ec, "bind", true);
    return;
  }

  // Start listening for connections
  _acceptor.listen(net::socket_base::max_listen_connections, ec);
  if (ec) {
    fail(ec, "listen", true);
    return;
  }
}

// Start accepting incoming connections
void Listener::run() { do_accept(); }

void Listener::do_accept() {
  // The new connection gets its own strand
  _acceptor.async_accept(
      net::make_strand(_ioc),
      beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
}
void Listener::on_accept(beast::error_code ec, tcp::socket socket) {
  if (ec) {
    fail(ec, "listener accept");
  } else {
    auto sessionState = std::make_shared<SessionState>();
    sessionState->uuid = uuids::to_string(_generator());

    sessionState->acceptCallback = [sessionState, this]() {
      sessionState->isConnected = true;
      _state->acceptCallback(sessionState);
    };
    sessionState->disconnectCallback = [sessionState, this]() {
      sessionState->isConnected = false;
      _state->disconnectCallback(sessionState);
    };
    sessionState->readCallback = [sessionState, this](const std::string &msg) {
      _state->readCallback(sessionState, msg);
    };
    sessionState->writeCallback = [sessionState, this](size_t len) {
      _state->writeCallback(sessionState, len);
    };

    sessionState->session =
        std::make_shared<Session>(std::move(socket), _ioc, sessionState);

    sessionState->session->run();
  }

  // Accept another connection
  do_accept();
}
