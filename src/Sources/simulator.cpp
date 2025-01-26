#include "simulator.h"
#include "plane.h"

std::vector<data::PlaneData> Simulator::getPlaneData() {
  std::lock_guard<std::shared_mutex> guard(*_state->mtx);
  std::vector<data::PlaneData> aircrafts;
  aircrafts.reserve(_state->planes.size());
  for (const auto &plane : _state->planes) {
    aircrafts.emplace_back(plane.getData());
  }
  return aircrafts;
}

void Simulator::_loop() {
  while (_runLoop) {
    // calculating time delta
    auto now = std::chrono::steady_clock::now();
    auto deltaDuration = std::chrono::duration<double>(now - _lastTime);
    _lastTime = now;

    double timeDelta = deltaDuration.count();

    // updating planes
    {
      std::lock_guard<std::shared_mutex> guard(*_state->mtx);
      for (auto &plane : _state->planes) {
        plane.update(2 * timeDelta);
      }
    }

    // sending the planes' positions
    std::vector<data::PlaneFlightData> flightData;
    flightData.reserve(_state->planes.size());
    for (const auto &plane : _state->planes) {
      flightData.emplace_back(plane.getFlightData());
    }
    json base = R"({"type":"positions","aircrafts":[]})"_json;
    base["aircrafts"] = flightData;
    _wsServer->broadcast(base.dump());
    // std::cerr << "broadcasting pos update:" << base.dump() << std::endl;

    auto n2 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(_state->updateInterval - (n2 - now));
  }
}
Simulator::Simulator(std::shared_ptr<SimulatorState> state,
                     std::shared_ptr<WebsocketServer> wsServer)
    : _state(state), _wsServer(wsServer) {}

bool Simulator::isRunning() { return running; }

void Simulator::stop() { _runLoop = false; }

void Simulator::wait() {
  if (!running) {
    _runLoop = false;
    return;
  }

  if (_workerThread.joinable()) {
    _workerThread.join();
  }
  running = false;
}

void Simulator::start() {
  if (running) {
    _runLoop = true;
    return;
  }

  std::cout << "starting plane simulation" << std::endl;
  running = true;
  _runLoop = true;
  _lastTime = std::chrono::steady_clock::now();

  _workerThread = std::thread(&Simulator::_loop, this);
}
