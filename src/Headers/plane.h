#pragma once
#include "advanced_ai.h"
#include "airportData.h"
#include "common.h"
#include "convertions.h"
#include "flightPlan.h"
#include "geoPos.h"
#include "planeConfig.h"
#include "vec.h"
#include "velocity.h"

#include <atomic>
#include <cmath>
#include <controlParam.h>

enum class MODE { AUTO, HDG, AUX, GRD, PLAYER };
enum class GRD_MODE {
  NONE,
  APPROACH,
  TAXI_OUT,
  IDLE,
  TAXI_IN,
  HOLD_RWY,
  TAXI_RWY,
  TAKEOFF
};

// TO DO:
//  fix advanced turning/pathfinding
//  landing, takoff, airport circle
//  ground behavior
//  finish order processing

namespace data {
struct PlaneInfo {
  int id;
  bool sim_id;
  bool isGrounded;
  double fuel;
  std::string airline;
  std::string flightNumber;
  std::string planeNumber;
  std::string callSign;
  std::string squawk;
  std::string model;
};

void to_json(json &j, const PlaneInfo &p);
void from_json(const json &j, PlaneInfo &p);

struct PlaneData {
  PlaneInfo info;
  Velocity vel;
  GeoPos<double> pos;
};

void to_json(json &j, const PlaneData &p);
void from_json(const json &j, PlaneData &p);

struct PlaneFlightData {
  int id;
  std::string squawk;
  double fuel;
  Velocity vel;
  GeoPos<double> pos;
  std::vector<GeoPos<double>> targets;
};

void to_json(json &j, const PlaneFlightData &p);
void from_json(const json &j, PlaneFlightData &p);
} // namespace data

class Plane {
public:
  std::string uuid{""};
  data::PlaneInfo _info;
  Velocity _vel;
  GeoPos<double> _pos;

  MODE mode;
  GRD_MODE grdMode;
  FlightSegment _target;

  controlParam _auxParam;
  bool _declaredEmergency = false;

  FlightPlan _flightPlan;
  std::shared_ptr<const PlaneConfig> config;

  void setData(const data::PlaneData &pd);
  data::PlaneData getData() const;

  void setFlightData(const data::PlaneFlightData &pd);
  data::PlaneFlightData getFlightData() const;

  Plane(const data::PlaneData &data, const FlightPlan &flightplan,
        std::shared_ptr<const PlaneConfig> configPointer);

  void update(float timeDelta);

  // Maybe should be a seprate his class/file
  void generateLandingWaypoints(RUNWAY approach, bool succesful,
                                double slopeAngle, double distance);
  void generateTaxiWaypoints(RUNWAY runway);
  void generateRunwayWaypoints(bool toFront);
  void generateTakeOffWaypoints(bool toFront, double slope);

  // order handling
  void setAltitude(float altitude);
  void setHeadpoint(GeoPos<double> point);
  void setHeading(float heading);
  void setVelocity(float vel);
  void setVerticalSpeed(float value);
  void setSquawk(const std::string &sq);
  void followFlightPlan();
  void enterHolding();
  void landing(std::string name);
  void landing(RUNWAY runway);
  void taxiToRunway(std::string name);
  void enterRunway();
  void touchAndGo(std::string name);
  void takeOff();
  void enterAirportLoop();
  void setFuel(float value);
  // order handling

  void setModePlayer();

private:
  void updateVelocity(float timeDelta);
  void updatePosition(float timeDelta);
  void updateFlightPlan(bool force = false, double margin = 40);
  void updateParameters(float timeDelta);

  double getTurnFactor();
  double findHeadingDelta(GeoPos<double> pos, GeoPos<double> targetPos);
  double getTurnRadius();
  bool checkMinRadius();
  void generateHelperWaypoints(FlightSegment targetSegment);
  void setAuxParam();
  std::vector<GeoPos<double>> getTargets() const;

  void addWaypoint(FlightSegment segment, bool toFront = false);
  void setModeHdg();
  void setModeAux();
  void setModeAuto();

  double getTrgVel();
  double getTrgAlt();
};
