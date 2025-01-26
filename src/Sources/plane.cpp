#include "plane.h"

namespace data {
// PlaneInfo parsing
void to_json(json &j, const PlaneInfo &p) {
  j = json{
      {"id", p.id},
      {"sim_id", p.sim_id},
      {"isGrounded", p.isGrounded},
      {"airline", p.airline},
      {"flight_number", p.flightNumber},
      {"plane_number", p.planeNumber},
      {"callsign", p.callSign},
      {"squawk", p.squawk},
      {"model", p.model},
  };
}

void from_json(const json &j, PlaneInfo &p) {
  j.at("id").get_to(p.id);
  j.at("isGrounded").get_to(p.isGrounded);
  j.at("airline").get_to(p.airline);
  j.at("flight_number").get_to(p.flightNumber);
  j.at("plane_number").get_to(p.planeNumber);
  j.at("callsign").get_to(p.callSign);
  j.at("squawk").get_to(p.squawk);
  auto it = j.find("model");
  if (it == j.end())
    return;
  j.at("model").get_to(p.model);
}
// PlaneInfo parsing

// PlaneData parsing
void to_json(json &j, const PlaneData &p) {
  j = json{{"velocity", p.vel}, {"position", p.pos}};
  j.merge_patch(json(p.info));
}

void from_json(const json &j, PlaneData &p) {
  j.get_to(p.info);
  j.at("velocity").get_to(p.vel);
  j.at("position").get_to(p.pos);
}
// PlaneData parsing
// 
 //PlaneFlightData parsing
 void to_json(json &j, const PlaneFlightData &p) {
   j = json{{"id", p.id},
            {"fuel", p.fuel},
            {"squawk", p.squawk},
            {"velocity", p.vel},
            {"position", p.pos},
            {"targets", p.targets}};
 }


// to_json function
//void to_json(json& j, const PlaneFlightData& p) {
//    j = json{ {"id", p.id},
//             {"squawk", p.squawk},
//             {"fuel", p.fuel},
//             {"velocity",
//              {
//                  {"direction", p.vel.heading},
//                  {"value", p.vel.value},
//              }},
//             {"position",
//              {{"latitude", p.pos.lat()},
//               {"longitude", p.pos.lon()},
//               {"altitude", p.pos.alt()}}},
//             {"targets", json::array()} };
//
//    for (const auto& target : p.targets) {
//        j["targets"].push_back({
//            {"latitude", target.lat()},
//            {"longitude", target.lon()},
//            {"altitude", target.alt()}
//        });
//    }
//}

void from_json(const json &j, PlaneFlightData &p) {
  j.at("id").get_to(p.id);
  j.at("squawk").get_to(p.squawk);
  j.at("velocity").get_to(p.vel);
  j.at("position").get_to(p.pos);
  j.at("targets").get_to(p.targets);

}
// PlaneFlightData parsing

} // namespace data

void Plane::setData(const data::PlaneData &pd) {
  // TODO: maybe implement a better function of setting the data
  this->_info = pd.info;
  this->_vel = pd.vel;
  this->_pos = pd.pos;
  // data::PlaneFlightData pd2;
}
data::PlaneData Plane::getData() const {
  return data::PlaneData{this->_info,
                         {ms2kts(this->_vel.value), -this->_vel.heading},
                         xy2geo(this->_pos)}; // coordinate conversion
}

void Plane::setFlightData(const data::PlaneFlightData &pd) {
  // TODO: maybe implement a better function of setting the data
  this->_pos = geo2xy(pd.pos);
  this->_vel.heading = pd.vel.heading;
  this->_vel.value = kts2ms(pd.vel.value);
  // this->_target.pos = geo2xy(pd.targets.front());
}
data::PlaneFlightData Plane::getFlightData() const {
  // <<<<<<< HEAD
  // this is a heavy implementation, shoud be moved to a seperate msg that is
  // sent only on target updates
  std::vector<GeoPos<double>> targets = getTargets();

  /*for (auto trg : targets) std::cout << trg << std::endl;
  std::cout << std::endl;*/
  return data::PlaneFlightData{
      this->_info.id,     this->_info.squawk,
      this->_info.fuel,   {ms2kts(this->_vel.value), -this->_vel.heading},
      xy2geo(this->_pos), targets};
}

Plane::Plane(const data::PlaneData &data, const FlightPlan &flightplan,
             std::shared_ptr<const PlaneConfig> configPointer) {
  this->_info = data.info;
  this->_vel = data.vel;
  this->_pos = geo2xy(data.pos);
  this->_pos.alt() =
      std::min(std::max(_pos.alt(), 0.0), configPointer->maxAltitude);
  this->_flightPlan = flightplan;

  this->config = configPointer;
  this->_auxParam.altChange = config->deafultClimbingSpeed;

  this->_auxParam.overwriteVel = false;
  this->_auxParam.overwriteAlt = false;

  setModeAuto();
  this->grdMode = GRD_MODE::NONE;

  if (data.info.isGrounded) {
    setModeAux();
    this->grdMode = GRD_MODE::IDLE;
    _auxParam.overwriteVel = true;
    _auxParam.vel.value = 0;
  } else {
    this->_vel.value = std::min(std::max(_vel.value, configPointer->minSpeed),
                                configPointer->maxSpeed);

    updateFlightPlan(true);
  }
}

void Plane::update(float timeDelta) {
  // <<<<<<< HEAD
  // Debug
  // std::cout << static_cast<std::underlying_type<GRD_MODE>::type>(grdMode) <<
  // std::endl; std::cout << _info.callSign << " target: " <<
  // xy2geo(_target.pos)
  //         << " dist: " << distance(_pos, _target.pos) << std::endl;
  // std::cout << "Hdg: " << rad2hdg(_vel.heading) << " Speed: " << _vel.value
  //         << std::endl;
  // std::cout << "Pos(GEO): " << xy2geo(_pos) << std::endl;
  // std::cout << "Pos (XY): " << _pos << std::endl << std::endl;
  if (mode == MODE::PLAYER || grdMode == GRD_MODE::IDLE)
    return;

  updateFlightPlan();
  updateParameters(timeDelta);
  updatePosition(timeDelta);
  updateVelocity(timeDelta);
}

void Plane::updateVelocity(float timeDelta) {
  // <<<<<<< HEAD
  // Velcoity
  double velDelta = getTrgVel() - _vel.value;
  _vel.value +=
      sgn(velDelta) *
      std::min(std::abs(velDelta), std::pow(config->accelerationFactor, 2) /
                                       _vel.value * timeDelta);

  /*if (grdMode == GRD_MODE::NONE || grdMode == GRD_MODE::APPROACH) {
    _vel.value = std::min(std::max(_vel.value, config->minSpeed), config->maxSpeed);
  } else {
    _vel.value = std::min(std::max(_vel.value, 0.0), config->taxiMaxSpeed);
  }*/

  // Heading
  double hdgDelta = findHeadingDelta(_pos, _target.pos);
  _vel.heading +=
      sgn(hdgDelta) * std::min(std::abs(hdgDelta), getTurnFactor() * timeDelta);
  _vel.heading = fixAngle(_vel.heading);
}

void Plane::updatePosition(float timeDelta) {
  // XY Position
  _pos.lat() += std::sin(_vel.heading) * _vel.value * timeDelta;
  _pos.lon() += std::cos(_vel.heading) * _vel.value * timeDelta;


  // Altitude
  double altitudeDelta = getTrgAlt() - _pos.alt();

  _pos.alt() += sgn(altitudeDelta) * std::min(std::abs(altitudeDelta),
                                              _auxParam.altChange * timeDelta);

  _pos.alt() = std::max(std::min(_pos.alt(), config->maxAltitude), 0.0);
}

void Plane::updateFlightPlan(bool force, double margin) {

  if (mode == MODE::HDG)
    return;

  if (grdMode != GRD_MODE::NONE && grdMode != GRD_MODE::APPROACH)
    margin = 5;

  if (force || distance(_pos, _target.pos) < margin) {
    if ((_flightPlan.route.size() == 0 && _flightPlan.auxiliary.size() == 0) ||
        (mode == MODE::AUX && _flightPlan.auxiliary.size() == 0)) {

      setModeHdg();
      return;
    }

    FlightSegment next;
    if (_flightPlan.auxiliary.size()) {
      next = _flightPlan.auxiliary.front();
      _flightPlan.auxiliary.pop_front();
    } else {
      next = _flightPlan.route.front();
      _flightPlan.route.pop_front();
    }

    if (mode == MODE::AUX && next.vel.value == 0.0) {
      _auxParam.overwriteVel = false;
    }

    if (next.interpolateAlt) {
      _auxParam.overwriteAlt = false;
      _flightPlan.interTrg = {_pos, _vel};
    }
    if (next.interpolateVel) {
      _auxParam.overwriteVel = false;
      _flightPlan.interTrg = {_pos, _vel};
    }

    if (next.useHeading) {

      std::cout << "Changing Target " << next.pos << std::endl;
      _flightPlan.vaildPathFound = false;

      generateHelperWaypoints(next);
    } else {
      addWaypoint(next, true);
      this->_target = next;
    }
  }
}

void Plane::updateParameters(float timeDelta) {
  if (grdMode == GRD_MODE::NONE || grdMode == GRD_MODE::APPROACH) {
    _info.fuel -= config->fuelConsumption * timeDelta;
  } else {
    _info.fuel -= config->fuelConsumption * timeDelta * 0.1;
  }

  if (grdMode == GRD_MODE::APPROACH && _pos.alt() <= 0.1) {
    grdMode = GRD_MODE::TAXI_OUT;
    _auxParam.overwriteAlt = false;
    _auxParam.overwriteVel = false;
  }
}

double Plane::getTrgVel() {
  if (_auxParam.overwriteVel || mode == MODE::HDG) {
    return _auxParam.vel.value;
  }

  if (mode != MODE::HDG && _target.interpolateVel) {
    return _flightPlan.interTrg.vel.value + (_target.vel.value - _flightPlan.interTrg.vel.value) /
      distance(_target.pos, _flightPlan.interTrg.pos) * distance(_pos, _flightPlan.interTrg.pos);

  }
  return _target.vel.value;
}

double Plane::getTrgAlt() {
  if (_auxParam.overwriteAlt || mode == MODE::HDG) {
    return _auxParam.alt;
  }
  if (_target.interpolateAlt) {
    return _flightPlan.interTrg.pos.alt() +
           (_target.pos.alt() - _flightPlan.interTrg.pos.alt()) /
               distance(_target.pos, _flightPlan.interTrg.pos) *
               distance(_pos, _flightPlan.interTrg.pos);
  }
  return _target.pos.alt();
}

void Plane::setModeHdg() {
  if (grdMode != GRD_MODE::NONE) {
    _auxParam.overwriteVel = true;
    _auxParam.vel.value = 0;
    if (grdMode == GRD_MODE::TAXI_OUT) {
      grdMode = GRD_MODE::IDLE;
    } else if (grdMode == GRD_MODE::TAXI_IN) {
      grdMode = GRD_MODE::HOLD_RWY;
    } else if (grdMode == GRD_MODE::TAXI_RWY) {
      grdMode = GRD_MODE::TAKEOFF;
    }
    return;
  }
  if (mode != MODE::HDG)
    setAuxParam();
  mode = MODE::HDG;
}

void Plane::setModeAux() {
  if (grdMode == GRD_MODE::APPROACH)
    grdMode = GRD_MODE::NONE;
  mode = MODE::AUX;
  _flightPlan.auxiliary.clear();
  setAuxParam();
}

void Plane::setModeAuto() {
  _flightPlan.auxiliary.clear();
  mode = MODE::AUTO;
}

void Plane::setModePlayer() { mode = MODE::PLAYER; }

double Plane::getTurnFactor() {
  if (grdMode != GRD_MODE::NONE && grdMode != GRD_MODE::APPROACH)
    return _vel.value / 33;

  double n = (_declaredEmergency) ? config->maxLoad : config->normalLoad;
  return 9.81 / _vel.value * std::sqrt(std::pow(n, 2) - 1);
}

double Plane::findHeadingDelta(GeoPos<double> pos, GeoPos<double> targetPos) {
  double targetHeading;

  if (mode == MODE::HDG)
    targetHeading = _auxParam.vel.heading;
  else {
    targetHeading = fixAngle(
        std::atan2(targetPos.lat() - pos.lat(), targetPos.lon() - pos.lon()));
  }

  // Check if turn is possible
  /* if (_mode != MODE::HDG && checkMinRadius())
     return 0;*/

  // Check if advanced pathfinding requires repositioning
  if (mode != MODE::HDG && !_flightPlan.vaildPathFound) {
    generateHelperWaypoints(_target);
    return 0;
  }

  double hdgDelta = targetHeading - _vel.heading;
  double altDelta = (2 * PI - std::abs(hdgDelta)) * -sgn(hdgDelta);
  if (std::abs(altDelta) < std::abs(hdgDelta))
    hdgDelta = altDelta;
  return hdgDelta;
}

double Plane::getTurnRadius() {
  if (grdMode != GRD_MODE::NONE && grdMode != GRD_MODE::APPROACH) {
    return 33;
  }

  double vel = std::max(_vel.value, getTrgVel());
  vel = std::min(std::max(vel, config->minSpeed), config->maxSpeed);
  double n = (_declaredEmergency) ? config->maxLoad : config->normalLoad;
  return 1.2 *
         std::sqrt(-std::pow(vel, 4) / (std::pow(G, 2) * (1 - std::pow(n, 2))));
}

bool Plane::checkMinRadius() {
  double n = (_declaredEmergency) ? config->maxLoad : config->normalLoad;
  double r = getTurnRadius();
  GeoPos<double> A = {{_pos.lat() + std::sin(_vel.heading + PI / 2) * r,
                       _pos.lon() + std::cos(_vel.heading + PI / 2) * r,
                       _target.pos.alt()}};
  GeoPos<double> B = {{_pos.lat() - std::sin(_vel.heading + PI / 2) * r,
                       _pos.lon() - std::cos(_vel.heading + PI / 2) * r,
                       _target.pos.alt()}};
  return (distance(_target.pos, A) < r || distance(_target.pos, B) < r);
}

// Check 4 circle non-overlaping match combinations and find
// coresponding 4 valid tangent lines
void Plane::generateHelperWaypoints(FlightSegment targetSegment) {
  targetSegment.useHeading = false;
  addWaypoint(targetSegment, true);

  GeoPos<double> tPos = targetSegment.pos;
  Velocity tVel = targetSegment.vel;

  double r = getTurnRadius();

  Waypoint start = {_pos.lon(), _pos.lat(), _vel.heading};
  Waypoint end = {tPos.lon(), tPos.lat(), tVel.heading};

  int pointCount = int(std::ceil(r / 25.0));
  auto route = generateShortestRoute(start, end, r, pointCount);

  if (route.size() == 0) {
    _flightPlan.vaildPathFound = false;
    return;
  }

  addWaypoint({{{route[0].y, route[0].x, tPos.alt()}}, tVel, false, false},
              true);
  for (int i = 1; i < route.size(); i++) {
    addWaypoint({{{route[i].y, route[i].x, tPos.alt()}}, tVel, false, false});
  }

  _flightPlan.vaildPathFound = true;
  addWaypoint(targetSegment);
  updateFlightPlan();
}

void Plane::generateLandingWaypoints(RUNWAY approach, bool succesful = true,
                                     double slopeAngle = 3,
                                     double distance = 5000) {

  std::array<FlightSegment, LANDING_SIZE> choice =
      (approach == RUNWAY::R10) ? landing10 : landing28;
  FlightSegment first = choice[0];

  double dir = first.vel.heading - PI;
  double alt = distance * std::tan(dgr2rad(slopeAngle));

  GeoPos<double> wpLand = {{first.pos.lat() + sin(dir) * distance,
                            first.pos.lon() + cos(dir) * distance, alt}};
  generateHelperWaypoints({wpLand, {_vel.value, first.vel.heading}});
  first.vel.value = config->landingSpeed;
  first.interpolateVel = true;
  addWaypoint(first);

  if (!succesful) {
    generateTakeOffWaypoints(false, 15);
    return;
  }

  ////break first variant
  dir = first.vel.heading;
  double brakeDist = config->landingDistance;
  GeoPos<double> wpStop = {{first.pos.lat() + sin(dir) * brakeDist,
                            first.pos.lon() + cos(dir) * brakeDist, 0}};
  // addWaypoint(FlightSegment{ wpStop, {config->taxiSpeed,0}, false, false,
  // true});

  int exitway = EXIT_START;
  if (approach == RUNWAY::R28 && wpStop.lon() < choice[exitway].pos.lon() ||
      approach == RUNWAY::R10 && wpStop.lon() > choice[exitway].pos.lon()) {
    exitway += EXIT_SIZE;
  };

  addWaypoint(
      {choice[exitway].pos, {config->taxiSpeed, 0}, false, false, true});
  for (int i = exitway + 1; i < exitway + EXIT_SIZE; i++) {
    addWaypoint({choice[i].pos, {config->taxiSpeed, 0}});
  }
  for (int i = EXIT_START + 2 * EXIT_SIZE; i < LANDING_SIZE - 1; i++) {
    addWaypoint({choice[i].pos, {config->taxiSpeed, 0}});
  }
  addWaypoint({choice[LANDING_SIZE - 1].pos, {0, 0}, false, false, true});
}

void Plane::generateTaxiWaypoints(RUNWAY runway) {
  std::array<FlightSegment, TAKEOFF_SIZE> choice =
      (runway == RUNWAY::R28) ? takeOff28 : takeOff10;

  addWaypoint({choice[0].pos, {config->taxiSpeed, 0}}, true);
  int last = TAKEOFF_SIZE - 1;
  for (int i = 1; i < last; i++) {
    addWaypoint({choice[i].pos, {config->taxiSpeed, 0}});
  }
  addWaypoint({choice[last].pos, {0, 0}, false, false, true});
}

void Plane::generateRunwayWaypoints(bool toFront = true) {
  FlightSegment choice =
      (_flightPlan.setRunway == RUNWAY::R28) ? exitOuter10_1 : exitOuter28_1;

  double dir = choice.vel.heading;
  addWaypoint({choice.pos, {config->taxiSpeed, dir}}, toFront);
  GeoPos<double> stop = {
      {choice.pos.lat() + sin(dir) * 20, choice.pos.lon() + cos(dir) * 20, 0}};
  addWaypoint({stop, {0, 0}, false, false, true});
}

void Plane::generateTakeOffWaypoints(bool toFront = true, double slope = 20) {

  FlightSegment choice =
      (_flightPlan.setRunway == RUNWAY::R28) ? exitOuter10_1 : exitOuter28_1;

  double dist = config->landingDistance;
  double dir = choice.vel.heading;
  GeoPos<double> takeOff = {{choice.pos.lat() + sin(dir) * dist,
                             choice.pos.lon() + cos(dir) * dist, 0}};
  if (toFront) {
    addWaypoint({takeOff, {config->takeOffSpeed, 0}, false, false, true},
                toFront);
  }

  dist *= 4;
  double alt = dist * std::tan(dgr2rad(slope));
  GeoPos<double> end = {{choice.pos.lat() + sin(dir) * dist,
                         choice.pos.lon() + cos(dir) * dist, alt}};
  addWaypoint({end, {config->cruiseSpeed, 0}, false, true, true});
}

void Plane::addWaypoint(FlightSegment segment, bool toFront) {
  if (toFront) {
    _target = segment;
  } else {
    _flightPlan.auxiliary.push_back(segment);
  }
}

std::vector<GeoPos<double>> Plane::getTargets() const {
  std::vector<GeoPos<double>> targets;
  if (mode == MODE::PLAYER || grdMode == GRD_MODE::IDLE)
    return targets;

  targets.reserve(_flightPlan.route.size() + _flightPlan.auxiliary.size() + 1);
  if (mode != MODE::HDG)
    targets.emplace_back(xy2geo(_target.pos));
  if (mode != MODE::HDG) {
    for (auto &seg : _flightPlan.auxiliary)
      targets.emplace_back(xy2geo(seg.pos));
  }
  if (mode == MODE::AUTO) {
    for (auto &seg : _flightPlan.route)
      targets.emplace_back(xy2geo(seg.pos));
  }
  return targets;
}

void Plane::setAuxParam() {
  _auxParam.vel.heading = _vel.heading;
  if (!_auxParam.overwriteVel)
    _auxParam.vel.value = _vel.value;
  if (!_auxParam.overwriteAlt)
    _auxParam.alt = _pos.alt();
}

// order handling
void Plane::setAltitude(float altitude) {
  if (grdMode != GRD_MODE::NONE)
    return;
  if (mode != MODE::HDG && _target.interpolateAlt)
    return;

  _auxParam.overwriteAlt = true;
  _auxParam.alt = ft2meter(altitude);
}

void Plane::setHeadpoint(GeoPos<double> point) {
  setModeAux();
  FlightSegment seg = { geo2xy(point), _target.vel};

  addWaypoint(seg, true);
  _auxParam.overwriteVel = true;
  _auxParam.vel.value = _target.vel.value;
}

void Plane::setHeading(float heading) {
  if (grdMode == GRD_MODE::APPROACH)
    grdMode = GRD_MODE::NONE;

  setModeHdg();
  _auxParam.vel.heading = hdg2rad(heading);
}

void Plane::setVelocity(float vel) {
  if (mode == MODE::AUX && _target.vel.value == 0.0) return;
  if (mode != MODE::HDG && _target.interpolateVel) return;

  _auxParam.overwriteVel = true;
  double val = kts2ms(vel);
  if (grdMode != GRD_MODE::NONE && grdMode != GRD_MODE::APPROACH) {
    val = std::max(std::min(val, config->taxiMaxSpeed), 0.0);
  } else {
    val = std::max(std::min(val, config->maxSpeed), config->minSpeed);
  }
  _auxParam.vel.value = val;

  if (grdMode == GRD_MODE::APPROACH) {
    landing(_flightPlan.setRunway);
  }
}

void Plane::setSquawk(const std::string &sq) { this->_info.squawk = sq; }

void Plane::followFlightPlan() {
  if (grdMode == GRD_MODE::NONE) {
    _flightPlan.auxiliary.clear();
    setModeAuto();
  }

  _auxParam.overwriteAlt = false;
  _auxParam.overwriteVel = false;
}

void Plane::landing(std::string name) {
  if (grdMode != GRD_MODE::NONE && grdMode != GRD_MODE::APPROACH) return;
  RUNWAY runway;
  if (name == "MAPt 28") runway = RUNWAY::R28;
  else if (name == "MAPt 10") runway = RUNWAY::R10;
  else return;


  _auxParam.overwriteAlt = false;
  _auxParam.overwriteVel = false;

  setModeAux();
  _flightPlan.setRunway = runway;
  generateLandingWaypoints(runway);
  grdMode = GRD_MODE::APPROACH;
}

void Plane::landing(RUNWAY runway) {

  setModeAux();
  _flightPlan.setRunway = runway;
  generateLandingWaypoints(runway);
  grdMode = GRD_MODE::APPROACH;
}

void Plane::touchAndGo(std::string name) {
  if (grdMode != GRD_MODE::NONE && grdMode != GRD_MODE::APPROACH)
    return;

  setModeAux();
  if (name == "MAPt 28") {
    _flightPlan.setRunway = RUNWAY::R28;
    generateLandingWaypoints(RUNWAY::R28, false);
  } else {
    _flightPlan.setRunway = RUNWAY::R10;
    generateLandingWaypoints(RUNWAY::R10, false);
  }
}
void Plane::taxiToRunway(std::string name) {
  if (grdMode != GRD_MODE::IDLE)
    return;

  setModeAux();
  _auxParam.overwriteAlt = false;
  _auxParam.overwriteVel = false;

  if (name == "MAPt 28") {
    _flightPlan.setRunway = RUNWAY::R28;
    generateTaxiWaypoints(RUNWAY::R28);
  } else {
    _flightPlan.setRunway = RUNWAY::R10;
    generateTaxiWaypoints(RUNWAY::R10);
  }
  grdMode = GRD_MODE::TAXI_IN;
}
void Plane::enterRunway() {
  if (grdMode == GRD_MODE::HOLD_RWY) {

    setModeAux();
    _auxParam.overwriteAlt = false;
    _auxParam.overwriteVel = false;

    generateRunwayWaypoints();
    grdMode = GRD_MODE::TAXI_RWY;
  }
}
void Plane::takeOff() {
  if (grdMode == GRD_MODE::TAKEOFF) {
    _auxParam.overwriteAlt = false;
    _auxParam.overwriteVel = false;

    setModeAux();
    generateTakeOffWaypoints();
    grdMode = GRD_MODE::NONE;
  }
}


void Plane::setFuel(float value) {

}

void Plane::setVerticalSpeed(float value) {
  _auxParam.altChange = ft2meter(value);
}


void Plane::enterAirportLoop() {}
void Plane::enterHolding() {}
// order handling
