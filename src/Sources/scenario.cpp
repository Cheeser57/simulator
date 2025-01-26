#include "scenario.h"
#include "convertions.h"

namespace data {
FlightSegment ScenarioWaypoint::asFlightSegment(const ScenarioWaypoint &wp) {
  // return FlightSegment{
  //     geo2xy(wp.position),
  //     {kts2ms(wp.velocity.value), hdg2rad(wp.velocity.heading)},
  //     false};
  return FlightSegment{
      wp.position, {wp.velocity.value, wp.velocity.heading}, false};
}
FlightSegment ScenarioWaypoint::asFlightSegment() const {
  return ScenarioWaypoint::asFlightSegment(*this);
}

// ScenarioWaypoint parsing
void to_json(json &j, const ScenarioWaypoint &p) {
  j = json{{"id", p.id},
           {"statusId", p.statusId},
           {"time", p.time},
           {"is_entrypoint", p.is_entrypoint},
           {"is_exit_point", p.is_exit_point},
           {"fuel_state", p.fuel_state},
           {"weather", p.weather},
           {"Events", p.events},
           {"position", p.position},
           {"velocity", p.velocity}};
};
void from_json(const json &j, ScenarioWaypoint &p) {
  j.at("id").get_to(p.id);
  j.at("statusId").get_to(p.statusId);
  j.at("time").get_to(p.time);
  j.at("is_entrypoint").get_to(p.is_entrypoint);
  j.at("is_exit_point").get_to(p.is_exit_point);
  j.at("fuel_state").get_to(p.fuel_state);
  j.at("weather").get_to(p.weather);
  j.at("Events").get_to(p.events);
  // j.at("position").get_to(p.position);
  // j.at("velocity").get_to(p.velocity);

  // conversion to the internal repr
  // p.position = geo2xy(p.position);
  // p.velocity = {kts2ms(p.velocity.value), hdg2rad(p.velocity.heading)};
};
// ScenarioWaypoint parsing

FlightPlan ScenarioFlight::asFlightPlan(const ScenarioFlight &sf) {
  FlightPlan plan;
  for (const auto &waypoint : sf.waypoints) {
    plan.route.push_front(waypoint.asFlightSegment());
  }
  return plan;
}
FlightPlan ScenarioFlight::asFlightPlan() const {
  return ScenarioFlight::asFlightPlan(*this);
}

// ScenarioFlight parsing
void to_json(json &j, const ScenarioFlight &p) {
  j = json{{"type_of_aircraft", p.type_of_aircraft},
           {"registrationnumber", p.registrationnumber},
           {"destination", p.destination},
           {"alternative_airports", p.alternative_airports},
           {"weather", p.weather},
           {"items", p.waypoints}};
  j.merge_patch(json(p.planeData));
};
void from_json(const json &j, ScenarioFlight &p) {
  j.get_to(p.planeData);
  j["type_of_aircraft"].get_to(p.type_of_aircraft);
  j["registrationnumber"].get_to(p.registrationnumber);
  j["destination"].get_to(p.destination);
  j["alternative_airports"].get_to(p.alternative_airports);
  j["weather"].get_to(p.weather);
  // j["items"].get_to(p.waypoints);
  return;
};
// ScenarioFlight parsing

std::vector<Plane> Scenario::getPlanes() {
  std::vector<Plane> planes;
  planes.reserve(flights.size());
  for (const auto &flight : flights) {
    // TODO: create PlaneConfig based on the type_of_aircraft
    std::shared_ptr<const PlaneConfig> configPtr =
        std::make_shared<const PlaneConfig>(
            // PlaneConfig{60.5, 241.9, 12000, 25, 20, 1.35, 2.56, 70, 30,
            // 1000});
            PlaneConfig{60.5, 241.9, 12000, 5, 25, 220, 10, 1.2, 2.56, 70, 2000,
                        80, 2500, kts2ms(20), kts2ms(30)});
    Plane p = Plane(flight.planeData, flight.asFlightPlan(), configPtr);
    // conversion to the internal repr
    // p._pos = geo2xy(p._pos);
    // p._vel = {kts2ms(p._vel.value), hdg2rad(p._vel.heading)};
    planes.emplace_back(p);
  }
  return planes;
}

// Scenario parsing
void to_json(json &j, const Scenario &p) { j = json(p.flights); };
void from_json(const json &j, Scenario &p) { j.get_to(p.flights); };
// Scenario parsing

} // namespace data
