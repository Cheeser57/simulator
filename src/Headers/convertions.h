#pragma once
#include "geoPos.h"
#include <cmath>

const double PI = 3.1415;
const double G = 9.81;
const double MAG_NORTH_DIFF = 0.1308996939;

double fixAngle(double angle);

double dgr2rad(double degrees);
double hdg2rad(double degrees);

double rad2dgr(double radian);
double rad2hdg(double degrees);

double meter2ft(double alt);
double ft2meter(double alt);

double meter2lat(double distance);

double meter2long(double distance, double latitude);

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

double distance(GeoPos<double> posA, GeoPos<double> posB);

double distance(std::pair<double, double> posA, std::pair<double, double> posB);

double ms2kts(double value);
double kts2ms(double value);

const double matrix[2][2] = {{1.11275852e+05, -5.11538036e-01},
                             {8.48524855e+00,  6.80367423e+04}};
const double matrixInv[2][2] = {{8.98667578e-06,  6.75668223e-11},
                                {-1.12077938e-09,  1.46979406e-05}};
const double matrixConst[2] = { -5.83307847e+06, - 1.14527664e+06 };

GeoPos<double> geo2xy(GeoPos<double> pos);

GeoPos<double> xy2geo(GeoPos<double> pos);

double parseDMS(const std::string &dms);
