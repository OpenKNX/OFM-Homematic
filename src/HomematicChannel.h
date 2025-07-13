// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#pragma once
#include "OpenKNX.h"

#include "HTTPClient.h"
#include <tinyxml2.h>


#define HMG_DEVTYPE_1 (1)
#define HMG_DEVTYPE_6 (6)

// KOs for device type 1: 
#define HMG_KoKOdTempSet          HMG_KoKOd__KO__8
#define HMG_KoKOdBoostTrigger     HMG_KoKOd__KO__10
#define KoHMG_KOdTempCurrent      KoHMG_KOd__KO__6
#define KoHMG_KOdTempSetCurrent   KoHMG_KOd__KO__7
#define KoHMG_KOdTempSet          KoHMG_KOd__KO__8
#define KoHMG_KOdBoostState       KoHMG_KOd__KO__9
#define KoHMG_KOdBoostTrigger     KoHMG_KOd__KO__10
#define KoHMG_KOdValveState       KoHMG_KOd__KO__11 

// KOs for device type 9:
#define HMG_KoKOdSwitch           HMG_KoKOd__KO__7
#define HMG_KoKOdSwitchStair      HMG_KoKOd__KO__8
#define HMG_KoKOdLock             HMG_KoKOd__KO__10
#define KoHMG_KOdSwitchState      KoHMG_KOd__KO__6
#define KoHMG_KOdSwitch           KoHMG_KOd__KO__7
#define KoHMG_KOdSwitchStair      KoHMG_KOd__KO__8
#define KoHMG_KOdLockState        KoHMG_KOd__KO__9
#define KoHMG_KOdLock             KoHMG_KOd__KO__10


class HomematicChannel : public OpenKNX::Channel
{
  private:
    // is enabled in ETS?
    bool _channelActive = false;

    // is started (when enabled and after startup-delay)
    bool _running = false;

    uint32_t _lastRequest_millis = 0;
    uint32_t _requestInterval_millis = 3600 * 1000;

    // is setting values allowed?
    bool _allowedWriting = true;

    bool _logResponse = false;

    bool update();
    bool updateRssi();
    tinyxml2::XMLElement* getMethodResponseMember(tinyxml2::XMLDocument &doc);
    bool updateKOsFromMethodResponse(tinyxml2::XMLDocument &doc);
    bool processRssiInfoResponse(tinyxml2::XMLDocument &doc);

    void sendSetTemperature(double targetTemperature);
    bool rpcSetValueDouble(const char * paramName, double value);
    void sendBoost(bool boost);
    bool rpcSetValueBool(const char * paramName, bool value);

    void requestAddParamDeviceSerial(arduino::String &request);
    void requestAddParamString(arduino::String &request, const char *str);
    void requestAddParamDouble(arduino::String &request, double value);
    void requestAddParamInteger4(arduino::String &request, int32_t i4Value);
    void requestAddParamBoolean(arduino::String &request, boolean Value);

    bool sendRequestGetResponseDoc(arduino::String &request, tinyxml2::XMLDocument &doc);
    bool sendRequestCheckResponseOk(arduino::String &request);
    bool checkSendRequestResponse(tinyxml2::XMLDocument &doc);

    void debugLogResponse(HTTPClient &http);

    void processInputKo_DevType1(GroupObject &ko);
    void processInputKo_DevType6(GroupObject &ko);

  public:
    explicit HomematicChannel(uint8_t index);
    const std::string name() override;
    void setup() override;
    void loop() override;
    void processAfterStartupDelay();
    void processInputKo(GroupObject &ko) override;

    bool processCommandOverview();
};