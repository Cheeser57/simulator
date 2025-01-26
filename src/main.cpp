#include "common.h"
#include "plane.h"
#include "scenario.h"
#include "simulator.h"
// #include "simulatorState.h"
#include "messageParsing.h"
#include "websocketServer.h"
#include <chrono>
#include <sstream>

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "Usage: simulator <address> <sim-port> <sim-threads>\n"
              << "Example:\n"
              << "    websocket-server 0.0.0.0 8080 1\n";
    // std::cerr << "Usage: simulator <address> <sim-port> <sim-threads> "
    //              "<voice-address> <voice-port> <voice-threads>\n"
    //           << "Example:\n"
    //           << "    websocket-server 0.0.0.0 8080 1 0.0.0.0 8081 1\n";
    return EXIT_FAILURE;
  }

  auto const address = std::string(argv[1]);
  auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
  auto const threads = std::max<int>(1, std::atoi(argv[3]));

  std::shared_ptr<WebsocketServer> websocketServer;
  std::unique_ptr<Simulator> simulator;
  std::shared_ptr<ServerState> serverState = std::make_shared<ServerState>();
  serverState->threads = threads;
  serverState->address = address;
  serverState->port = port;

  // Testing stuff
  std::shared_ptr<SimulatorState> simState = std::make_shared<SimulatorState>(
      SimulatorState{std::move(std::make_unique<std::shared_mutex>()),
                     {},
                     {},
                     std::chrono::microseconds(16666)});

  // This should be loaded into map from map and used when loading scenario
  // std::shared_ptr<const PlaneConfig> configPtr =
  //     std::make_shared<const PlaneConfig>(
  //         PlaneConfig{40, 100, 8000, 3, 20, 150, 10, 1.3, 2.56, 50, 1000, 55,
  //                     1200, kts2ms(20), kts2ms(30)}); // in sim units

  // std::shared_ptr<const PlaneConfig> configPtr2 =
  //     std::make_shared<const PlaneConfig>(
  //         PlaneConfig{60.5, 241.9, 12000, 5, 25, 220, 10, 1.2, 2.56, 70,
  //         2000,
  //                     80, 2500, kts2ms(20), kts2ms(30)}); // in sim units

  // FlightPlan plan0;
  // simState->planes.push_back(
  //     Plane(data::PlaneData{{0, true, false, 10000, "LOT", "286", "SP-LVN",
  //                            "PLAYER", "2500", "Mooney Bravo"},
  //                           Velocity{0, hdg2rad(-50)},
  //                           xy2geo(exitInner10_2.pos)},
  //           plan0, configPtr));

  // FlightPlan plan;
  // plan.route.push_back(
  //     FlightSegment{geo2xy(GeoPos<double>{{52.42, 16.82, 10000.0}}),
  //                   Velocity{80, hdg2rad(0)}});

  // simState->planes.front().setModePlayer();

  // simState->planes.push_back(
  //     Plane(data::PlaneData{{1, false, true, 10000, "LOT", "286", "SP-LVN",
  //                            "LOT286", "2000", "Mooney Bravo"},
  //                           Velocity{100, hdg2rad(-30)},
  //                           {{52.4, 16.8, 1000}}},
  //           plan, configPtr2));

  // FlightPlan plan2;
  // plan2.route.push_back(
  //     FlightSegment{geo2xy(GeoPos<double>{{52.5, 16.5, 10000.0}}),
  //                   Velocity{100, hdg2rad(270)}, true, false});

  // simState->planes.push_back(
  //     Plane(data::PlaneData{{2, false, 10000, "LOT", "287", "SP-XTZ",
  //     "LOT287",
  //                            "2010", "Mooney Bravo"},
  //                           Velocity{100, hdg2rad(0)},
  //                           {{52.4, 16.45, 1500}}},
  //           plan2, configPtr2));

  // simState->planes.push_back(
  //     Plane(data::PlaneData{{3, false, 1000, "", "", "SP-256", "SP-256",
  //     "2010",
  //                            "Mooney Bravo"},
  //                           Velocity{100, hdg2rad(270)},
  //                           {{52.41, 16.89, 2000}}},
  //           plan2, configPtr));

  // simState->planes.push_back(
  //     Plane(data::PlaneData{{4, true, 1000, "", "", "SP-001", "SP-001",
  //     "2000",
  //                            "Mooney Bravo"},
  //                           Velocity{0, hdg2rad(-20)},
  //                           {{52.416081, 16.827386, 0}}},
  //           plan, configPtr));

  // End of testing stuff
  serverState->acceptCallback = [&websocketServer,
                                 &simulator](auto sessionState) {
    websocketServer->newSession(sessionState);
    std::cout << "New client with uuid: " << sessionState->uuid << std::endl;
    if (!simulator->running) {
      return;
    }
    const auto &aircrafts = simulator->getPlaneData();
    json base = R"({"type":"start","aircrafts":[]})"_json;
    base["aircrafts"] = aircrafts;

    // find the sim_id id
    const auto it = std::find_if(
        aircrafts.begin(), aircrafts.end(),
        [](const data::PlaneData &p) { return p.info.sim_id == true; });
    base["sim_id"] = -1;
    if (it != aircrafts.end()) {
      base["sim_id"] = it->info.id;
    }
    // >>>>>>> main
    websocketServer->send(sessionState->uuid, base.dump());
    std::cerr << "sending init(to: " << sessionState->uuid
              << "): " << base.dump() << std::endl;
  };
  serverState->disconnectCallback = [&websocketServer](auto sessionState) {
    websocketServer->deleteSession(sessionState->uuid);
    std::cout << "Client disconnected with uuid: " << sessionState->uuid
              << std::endl;
  };

  serverState->readCallback = [&websocketServer, &simState, &simulator](
                                  std::shared_ptr<SessionState> sessionState,
                                  const std::string &msg) {
    std::cout << "Message from " << sessionState->uuid << " : " << msg
              << std::endl;
    if (sessionState->nextMsgIsScenario) {
      sessionState->nextMsgIsScenario = false;
      data::Scenario scenario;
      try {
        scenario = json::parse(msg);
      } catch (const std::exception &e) {
        std::cerr << "Error parsing the scenario file: " << e.what()
                  << std::endl;
        websocketServer->send(sessionState->uuid,
                              R"({"error":"Couldn't parse the scenario."})");
        return;
      }

      simState->planes = scenario.getPlanes();
      // simState

      const auto &aircrafts = simulator->getPlaneData();
      json base = R"({"type":"start","aircrafts":[]})"_json;
      base["sim_id"] = 0; // !!! tu wartosc sim_id z scenariusza
      base["aircrafts"] = aircrafts;
      // find the sim_id id
      const auto it = std::find_if(
          aircrafts.begin(), aircrafts.end(),
          [](const data::PlaneData &p) { return p.info.sim_id == true; });
      base["sim_id"] = -1;
      if (it != aircrafts.end()) {
        base["sim_id"] = it->info.id;
      }

      std::cerr << "sending init broadcast: " << base.dump() << std::endl;
      websocketServer->broadcast(base.dump());

      // start the simulator here upon receiving the scenario
      simulator->start();
      return;
    }
    try {
      json jsonMsg = json::parse(msg);
      // lock the state to make sure it doesn't change during parsing
      std::lock_guard<std::shared_mutex> guard(*simState->mtx);
      // parse message here
      messageParsing::parseMessage(jsonMsg, simState, sessionState);
      return;
      // >>>>>>> main
    } catch (const json::parse_error &er) {
      std::cerr << "Invalid json message" << std::endl;
    } catch (const json::type_error &er) {
      std::cerr << "Wrong type used during parsing: " << er.what() << std::endl;
    }
    websocketServer->send(sessionState->uuid,
                          R"({"error":"Couldn't parse the json message."})");
  };
  serverState->writeCallback = [](auto sessionState, size_t len) {};
  websocketServer = std::make_shared<WebsocketServer>(serverState);
  simulator = std::make_unique<Simulator>(simState, websocketServer);

  websocketServer->run();
  // for testing
  // simulator->start();

  websocketServer->wait();
  simulator->wait();

  return EXIT_SUCCESS;
}
