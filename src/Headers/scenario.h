#pragma once

#include "common.h"
#include "flightPlan.h"
#include "geoPos.h"
#include "plane.h"
#include "velocity.h"

namespace data {
struct ScenarioWaypoint {
  int id;
  int statusId;
  int time;
  bool is_entrypoint;
  bool is_exit_point;
  double fuel_state;
  std::string weather;
  std::string events;
  GeoPos<double> position;
  Velocity velocity;

  static FlightSegment asFlightSegment(const ScenarioWaypoint &wp);
  FlightSegment asFlightSegment() const;
};

void to_json(json &j, const ScenarioWaypoint &p);
void from_json(const json &j, ScenarioWaypoint &p);

// Struct to represent flight data
struct ScenarioFlight {
  // PlaneInfo info;
  PlaneData planeData;
  std::string type_of_aircraft;
  std::string registrationnumber;
  std::string destination;
  std::string alternative_airports;
  std::string weather;
  std::vector<ScenarioWaypoint> waypoints;

  static FlightPlan asFlightPlan(const ScenarioFlight &sf);
  FlightPlan asFlightPlan() const;
};

void to_json(json &j, const ScenarioFlight &p);
void from_json(const json &j, ScenarioFlight &p);

// Struct to represent the entire scenario
struct Scenario {
  std::vector<ScenarioFlight> flights;

  std::vector<Plane> getPlanes();
};

void to_json(json &j, const Scenario &p);
void from_json(const json &j, Scenario &p);

} // namespace data
