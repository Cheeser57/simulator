#include "session.h"

Session::Session(tcp::socket &&socket, net::io_context &ioc,
                 std::shared_ptr<SessionState> state)
    : _ws(std::move(socket)), _strand(net::make_strand(ioc)), _state(state) {}

// Start the asynchronous operation
void Session::run() {
  _ws.set_option(
      websocket::stream_base::timeout::suggested(beast::role_type::server));

  _ws.set_option(
      websocket::stream_base::decorator([](websocket::response_type &res) {
        res.set(http::field::server, "simulator websocket-server");
      }));

  auto self = shared_from_this();
  http::async_read(_ws.next_layer(), _recvBuffer, _req,
                   [self](beast::error_code ec, std::size_t bytes_transferred) {
                     self->on_http_read(ec, bytes_transferred);
                   });

  // _ws.async_accept(
  //     beast::bind_front_handler(&Session::on_accept, shared_from_this()));
}
void Session::on_http_read(beast::error_code ec,
                           std::size_t bytes_transferred) {
  if (ec) {
    std::cerr << "HTTP Read Error: " << ec.message() << "\n";
    return;
  }

  // Access the target (path + query string)
  std::string target = _req.target();
  std::cout << "HTTP Target: " << target << "\n";

  std::string query_string;
  auto pos = target.find('?');
  if (pos != std::string::npos) {
    query_string = target.substr(pos + 1); // Get the query part after '?'
  }

  // Parse the query string
  try {
    auto parsed_query = urls::parse_query(query_string);
    if (parsed_query.has_value()) {
      for (auto key : *parsed_query) {
        std::cout << key.key << " : " << key.value << std::endl;
      }

      // TODO: discuss whether it's a good idea to track sessions
      // // clientId parameter
      // auto client_id = parsed_query->find("clientId");
      // if (client_id != parsed_query->end()) {
      //   std::cout << "Client ID: " << client_id->value << "\n";
      // } else {
      //   std::cerr << "clientId parameter not found\n";
      // }

      // // program type parameter
      // auto it = parsed_query->find("programType");
      // if (it != parsed_query->end()) {
      //   std::cout << "programType: " << it->value << "\n";
      //   std::string pt = std::string(it->value);
      //   _state->clientType = ClientType::fromString(pt);
      // } else {
      //   std::cerr << "programType parameter not found\n";
      // }
    }
  } catch (const std::exception &ex) {
    std::cerr << "Error parsing query: " << ex.what() << "\n";
  }

  _ws.async_accept(
      _req, beast::bind_front_handler(&Session::on_accept, shared_from_this()));
}
void Session::on_accept(beast::error_code ec) {
  if (ec) {
    return fail(ec, "session accept");
  }
  _state->acceptCallback();
  do_read();
}

void Session::send(const std::string &message) {
  auto buffer = std::make_shared<beast::flat_buffer>();
  auto mutable_buffer = buffer->prepare(message.size());
  std::memcpy(mutable_buffer.data(), message.data(), message.size());
  buffer->commit(message.size());

  net::post(_strand, [self = shared_from_this(), buffer]() {
    self->_writeQueue.push_back(buffer);
    if (self->_writeQueue.size() == 1) {
      self->do_write();
    }
  });
}

void Session::do_write() {
  if (_writeQueue.empty())
    return;

  auto buffer = _writeQueue.front();
  _ws.async_write(buffer->data(),
                  [self = shared_from_this()](beast::error_code ec,
                                              std::size_t bytes_transferred) {
                    self->on_write(ec, bytes_transferred);
                  });
}

void Session::do_read() {
  net::post(_strand, [self = shared_from_this()]() {
    self->_ws.async_read(self->_recvBuffer,
                         beast::bind_front_handler(&Session::on_read, self));
  });
}

void Session::on_read(beast::error_code ec, std::size_t bytesTransferred) {
  if (ec == websocket::error::closed || ec == net::error::eof) {
    fail(ec, "session read");
    _state->disconnectCallback();
    return;
  }

  if (ec) {
    return fail(ec, "session read");
  }

  if (_state->nextMsgIsScenario == true) {
    _ws.text(_ws.got_binary());
  } else {
    _ws.text(_ws.got_text());
  }

  std::string msg(beast::buffers_to_string(_recvBuffer.cdata()));
  _recvBuffer.consume(bytesTransferred);
  _state->readCallback(msg);

  do_read();

  static bool first = true;
  if (first) {
    first = false;
  }
}

// void Session::on_write(beast::error_code ec, std::size_t bytesTransferred) {
//   if (ec) {
//     fail(ec, "session write");
//     _state->disconnectCallback();
//     return;
//   }

//   _state->writeCallback(bytesTransferred);
// }

void Session::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    fail(ec, "session write");
    _state->disconnectCallback();
    return;
  }

  _state->writeCallback(bytes_transferred);
  net::post(_strand, [self = shared_from_this()]() {
    self->_writeQueue.pop_front();
    if (!self->_writeQueue.empty()) {
      self->do_write();
    }
  });
}
