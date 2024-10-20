// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024 Cornelius Koepp

#pragma once
#include "OpenKNX.h"

#include "HTTPClient.h"
#include <tinyxml2.h>

class HomematicChannel : public OpenKNX::Channel
{
  private:
    // is enabled in ETS?
    bool _channelActive = false;

    // is started (when enabled and after startup-delay)
    bool _running = false;

    uint32_t _lastRequest_millis = 0;

    // is setting values allowed?
    bool _allowedWriting = true;

    bool update();
    tinyxml2::XMLElement* getMethodResponseMember(tinyxml2::XMLDocument &doc);
    bool updateKOsFromMethodResponse(tinyxml2::XMLDocument &doc);
    bool processRssiInfoResponse(tinyxml2::XMLDocument &doc);
    void sendSetTemperature(double targetTemperature);
    void sendBoost(bool boost);

    void requestAddParamDeviceSerial(arduino::String &request);
    void requestAddParamString(arduino::String &request, const char *str);
    void requestAddParamDouble(arduino::String &request, double value);
    void requestAddParamInteger4(arduino::String &request, int32_t i4Value);
    void requestAddParamBoolean(arduino::String &request, boolean Value);

    bool sendRequest(arduino::String &request);
    bool checkSendRequestResponse(tinyxml2::XMLDocument &doc);

    void debugLogResponse(HTTPClient &http);

  public:
    explicit HomematicChannel(uint8_t index);
    const std::string name() override;
    void setup() override;
    void loop() override;
    void processAfterStartupDelay();
    void processInputKo(GroupObject &ko) override;

    bool processCommandOverview();
};