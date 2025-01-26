#include "websocketServer.h"

WebsocketServer::WebsocketServer(std::shared_ptr<ServerState> state)
    : _state(state), _ioc(net::io_context{state->threads}) {
  auto const addr = net::ip::make_address(_state->address);
  // Create and launch a listening port
  auto listener = std::make_shared<Listener>(
      _ioc, tcp::endpoint{addr, _state->port}, _state);

  listener->run();
}

void WebsocketServer::newSession(std::shared_ptr<SessionState> session) {
  _state->sessions.emplace(session->uuid, session);
}

std::shared_ptr<SessionState>
WebsocketServer::getSession(const std::string &uuid) {
  auto it = _state->sessions.find(uuid);
  if (it == _state->sessions.end()) {
    return nullptr;
  }
  return it->second;
}

bool WebsocketServer::deleteSession(const std::string &uuid) {
  auto it = _state->sessions.find(uuid);
  if (it == _state->sessions.end()) {
    return false;
  }
  _state->sessions.erase(it);
  return true;
}

bool WebsocketServer::send(std::string uuid, const std::string &data) {
  auto it = _state->sessions.find(uuid);
  if (it == _state->sessions.end()) {
    return false;
  }
  it->second->session->send(data);
  return true;
}

void WebsocketServer::broadcast(const std::string &data,
                                const std::vector<std::string> exceptUuids) {
  for (const auto &[key, val] : _state->sessions) {
    // find uuid in exceptions
    auto it = std::find(exceptUuids.begin(), exceptUuids.end(), val->uuid);
    if (it != exceptUuids.end()) {
      continue;
    }
    val->session->send(data);
  }
}

void WebsocketServer::wait() {
  if (!isRunning)
    return;

  for (std::thread &t : _threads) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void WebsocketServer::run() {
  if (isRunning)
    return;

  isRunning = true;
  for (int i = 0; i < _state->threads; i++)
    _threads.emplace_back([this] { _ioc.run(); });
}
