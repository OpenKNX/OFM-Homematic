// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#pragma once
#include "OpenKNX.h"

#include "HTTPClient.h"
#include <tinyxml2.h>


#define HMG_DEVTYPE__HM_CC_RT_DN__THERMOSTAT (1)
#define HMG_DEVTYPE__HM_LC_Sw1_Pl_DN_R1__SWITCH_ACTUATOR (6)


// Helper macros for XML element checking
#define CHECK_RETURN(element, name, result) \
    if (element == nullptr) { \
        logErrorP("Element %s is missing!", name); \
        return result; \
    }

#define CHECK_NULL(element, name) CHECK_RETURN(element, name, nullptr);
#define CHECK_FALSE(element, name) CHECK_RETURN(element, name, false);


/**
 * Abstract base class for Homematic channels providing common functionality
 * for XML-RPC communication, parameter handling, and basic device management.
 */
class HomematicChannel : public OpenKNX::Channel
{
  protected:
    // Channel state
    bool _channelActive = false;
    bool _running = false;
    bool _allowedWriting = true;
    bool _logResponse = false;

    // Timing
    uint32_t _lastRequest_millis = 0;
    uint32_t _requestInterval_millis = 3600 * 1000;

    // Common device operations
    bool update();
    bool updateRssi();
    inline void updateRequestTiming(uint16_t intervalInSeconds);
    
    // XML-RPC communication
    tinyxml2::XMLElement* getMethodResponseMember(tinyxml2::XMLDocument &doc);
    bool updateKOsFromMethodResponse(tinyxml2::XMLDocument &doc, const uint8_t channel);
    bool processRssiInfoResponse(tinyxml2::XMLDocument &doc);
    bool sendRequestGetResponseDoc(arduino::String &request, tinyxml2::XMLDocument &doc);
    bool sendRequestCheckResponseOk(arduino::String &request);
    bool checkSendRequestResponse(tinyxml2::XMLDocument &doc);
    void debugLogResponse(HTTPClient &http);
    
    // XML-RPC parameter building
    void requestAddParamAddress(arduino::String &request, uint8_t channel);
    void requestAddParamString(arduino::String &request, const char *str);
    void requestAddParamDouble(arduino::String &request, double value);
    void requestAddParamInteger4(arduino::String &request, int32_t i4Value);
    void requestAddParamBoolean(arduino::String &request, boolean value);
    
    // XML-RPC RPC value setting
    bool rpcSetValueDouble(const uint8_t channel, const char * paramName, double value);
    bool rpcSetValueBool(const uint8_t channel, const char * paramName, bool value);

    // handler-methods by value-type
    virtual bool processResponseParamDouble(uint8_t channel, const char* pName, double value) { return false; }
    virtual bool processResponseParamInt32(uint8_t channel, const char* pName, int32_t value) { return false; }
    virtual bool processResponseParamBool(uint8_t channel, const char* pName, bool value) { return false; }

    // Pure virtual methods - to be implemented in child classes
    virtual void processDeviceSpecificInputKo(GroupObject &ko) = 0;
    virtual uint8_t getDeviceChannel() const = 0;

  public:
    explicit HomematicChannel(uint8_t index);
    virtual ~HomematicChannel() = default;
    
    const std::string name() override;
    void setup() override;
    void loop() override;
    void processAfterStartupDelay();
    void processInputKo(GroupObject &ko) override;

    bool processCommandOverview();
};
