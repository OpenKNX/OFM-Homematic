// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#include "HomematicChannelInactive.h"

HomematicChannelInactive::HomematicChannelInactive(uint8_t index) 
    : HomematicChannel(index)
{
}

const std::string HomematicChannelInactive::name()
{
    return "HMG-Inactive";
}

void HomematicChannelInactive::setup()
{
    // => _channelActive = false;
    // => _running = false;
    logDebugP("NO device configured");
}

void HomematicChannelInactive::loop()
{
    // NO-OP for inactive channels
}

void HomematicChannelInactive::processDeviceSpecificInputKo(GroupObject &ko)
{
    // NO-OP for inactive channels
}
