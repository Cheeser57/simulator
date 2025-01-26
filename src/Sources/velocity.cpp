#include "velocity.h"
#include "common.h"

void to_json(json &j, const Velocity &vel) {
  j = json{{"direction", vel.heading}, {"value", vel.value}};
}

void from_json(const json &j, Velocity &vel) {
  using namespace nlohmann::detail;

  // scenario sometimes encodes it as a string(?)
  switch (j.at("value").type()) {
  case value_t::string: {
    vel.value = std::stod(j.at("value").template get<std::string>());
    break;
  }
  case value_t::number_integer:
  case value_t::number_unsigned:
  case value_t::number_float: {
    j.at("value").get_to(vel.value);
    break;
  }
  default: {
    std::cerr << "Unknown velocity value type." << std::endl;
    return;
  }
  }
  auto it = j.find("heading");
  if (it != j.end()) {
    j.at("heading").get_to(vel.heading);
  } else {
    // scenario sometimes encodes it as a string(?)
    j.at("direction").get_to(vel.heading);
    switch (j.at("direction").type()) {
    case value_t::string: {
      vel.value = std::stod(j.at("direction").template get<std::string>());
      break;
    }
    case value_t::number_integer:
    case value_t::number_unsigned:
    case value_t::number_float: {
      j.at("direction").get_to(vel.heading);
      break;
    }
    default: {
      std::cerr << "Unknown velocity heading type." << std::endl;
      return;
    }
    }
  }
}
