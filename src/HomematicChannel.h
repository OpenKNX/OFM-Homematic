// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024 Cornelius Koepp

#pragma once
#include "OpenKNX.h"

#include <tinyxml2.h>

class HomematicChannel : public OpenKNX::Channel
{
  private:
    // is enabled in ETS?
    bool _channelActive = false;

    // is started (when enabled and after startup-delay)
    bool _running = false;

    uint32_t _lastRequest_millis = 0;

    void update();
    void updateKOsFromMethodResponse(tinyxml2::XMLDocument &doc);
    void sendSetTemperature(double targetTemperature);
    void sendBoost(bool boost);

    bool sendRequest(arduino::String &request);

public:
    explicit HomematicChannel(uint8_t index);
    const std::string name() override;
    void setup() override;
    void loop() override;
    void processAfterStartupDelay();
    void processInputKo(GroupObject &ko) override;

    bool processCommandOverview();
};