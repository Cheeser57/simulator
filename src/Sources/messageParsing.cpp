#include "messageParsing.h"

namespace messageParsing {
void order(const json &msg, std::shared_ptr<SimulatorState> simState) {
  int id = msg["id"];
  auto it = std::find_if(simState->planes.begin(), simState->planes.end(),
                         [id](const Plane &p1) { return p1._info.id == id; });
  if (it == simState->planes.end()) {
    std::cerr << "Plane from the given order not found! id: " << id
              << std::endl;
    return;
  }

  std::string orderType = msg["order_type"];

  std::unordered_map<std::string, const std::function<void()>> orderTypeMap = {
      {"altitude", [it, &msg]() { it->setAltitude(msg["data"]["value"]); }},
      {"headtopoint",
       [it, &msg]() {
         double lat = msg["data"]["lat"];
         double lon = msg["data"]["long"];
         it->setHeadpoint(GeoPos<double>{{lat, lon, -1}});
       }},
      {"heading", [it, &msg]() { it->setHeading(msg["data"]["value"]); }},
      {"velocity", [it, &msg]() { it->setVelocity(msg["data"]["value"]); }},
      {"squawk", [it, &msg]() { it->setSquawk(msg["data"]["value"]); }},
      {"follow_flight_plan", [it, &msg]() { it->followFlightPlan(); }},
      {"enter_holding",
       [it, &msg]() { it->taxiToRunway(msg["data"]["name"]); }},
      {"landing", [it, &msg]() { it->landing(msg["data"]["name"]); }},
      {"touch_and_go", [it, &msg]() { it->touchAndGo(msg["data"]["name"]); }},
      {"enter_runway", [it]() { it->enterRunway(); }},
      {"enter_airport_loop", [it]() { it->takeOff(); }}};

  if (orderTypeMap.find(orderType) != orderTypeMap.end()) {
    orderTypeMap[orderType]();
  } else {
    std::cerr << "Unknown order type: " << orderType << std::endl;
    return;
  }
}
void positions(const json &msg, std::shared_ptr<SimulatorState> simState) {
  // std::vector<data::PlaneData> planeData = msg["aircrafts"];
  // for (Plane &plane : simState->planes) {
  //   auto it = std::find_if(planeData.begin(), planeData.end(),
  //                          [&plane](const data::PlaneData &p) {
  //                            return p.info.id == plane._info.id;
  //                          });
  //   if (it != planeData.end()) {
  //     plane.setData(*it);
  //   }
  // }
  //       } else if (msgType == "positions") {
  //         // TODO: update the simulation with the "real" planes's
  std::vector<data::PlaneFlightData> planeData =
      msg["aircrafts"].template get<std::vector<data::PlaneFlightData>>();
  for (Plane &plane : simState->planes) {
    auto it = std::find_if(planeData.begin(), planeData.end(),
                           [&plane](const data::PlaneFlightData &p) {
                             return p.id == plane._info.id;
                           });
    if (it != planeData.end()) {
      plane.setFlightData(*it);
    }
  }
}

void parseMessage(const json &msg, std::shared_ptr<SimulatorState> simState,
                  std::shared_ptr<SessionState> sessionState) {
  std::unordered_map<std::string, std::function<void()>> msgTypeMap{
      {"scenario",
       [&sessionState]() { sessionState->nextMsgIsScenario = true; }},
      {"positions", [&msg, &simState]() { positions(msg, simState); }},
      {"order", [&msg, &simState]() { order(msg, simState); }}};

  std::string msgType = msg["type"];
  auto it = msgTypeMap.find(msgType);
  if (msgTypeMap.find(msgType) != msgTypeMap.end()) {
    msgTypeMap[msgType]();
  } else {
    std::cerr << "Unknown message type: " << msgType << std::endl;
  }
}

} // namespace messageParsing
