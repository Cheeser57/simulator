#include "common.h"
#include "vec.h"

#include "convertions.h"
#include "geoPos.h"

void to_json(json &j, const GeoPos<double> &p) {
  j = json{
      {"latitude", p.lat()}, {"longitude", p.lon()}, {"altitude", p.alt()}};
}

void from_json(const json &j, GeoPos<double> &p) {
  using namespace nlohmann::detail;

  // handle lat and long string types
  switch (j.at("latitude").type()) {
  case value_t::string: {
    p.lat() = parseDMS(j.at("latitude"));
    break;
  }
  case value_t::number_integer:
  case value_t::number_unsigned:
  case value_t::number_float: {
    j.at("latitude").get_to(p.lat());
    break;
  }
  default: {
    std::cerr << "Unknown geopos latitude type." << std::endl;
    return;
  }
  }
  switch (j.at("longitude").type()) {
  case value_t::string: {
    p.lon() = parseDMS(j.at("longitude"));
    break;
  }
  case value_t::number_integer:
  case value_t::number_unsigned:
  case value_t::number_float: {
    j.at("longitude").get_to(p.lon());
    break;
  }
  default: {
    std::cerr << "Unknown geopos longitude type." << std::endl;
    return;
  }
  }
  j.at("altitude").get_to(p.alt());
}
