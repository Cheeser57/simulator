#pragma once

#include "common.h"
#include "sessionState.h"
#include "simulatorState.h"

namespace messageParsing {
void order(const json &msg, std::shared_ptr<SimulatorState> simState);
void positions(const json &msg, std::shared_ptr<SimulatorState> simState);
void parseMessage(const json &msg, std::shared_ptr<SimulatorState> simState,
                  std::shared_ptr<SessionState> sessionState);
} // namespace messageParsing
