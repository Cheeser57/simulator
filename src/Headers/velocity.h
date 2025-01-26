#pragma once

#include "common.h"

struct Velocity {
  double value;   // ground speed in m/s
  double heading; // rad
};

void to_json(json &j, const Velocity &vel);
void from_json(const json &j, Velocity &vel);
