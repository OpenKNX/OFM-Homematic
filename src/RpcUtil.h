// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <tinyxml2.h>
#include <string>
#include "OpenKNX.h"
#include "OpenKNX/Base.h"


// Helper macros for XML element checking
#define CHECK_RETURN(element, name, result) \
    if (element == nullptr) { \
        logErrorP("Element %s is missing!", name); \
        return result; \
    }

#define CHECK_NULL(element, name) CHECK_RETURN(element, name, nullptr);
#define CHECK_FALSE(element, name) CHECK_RETURN(element, name, false);


/**
 * @brief Helper class for Homematic XML-RPC communication
 * 
 * This class provides methods for XML-RPC request building, 
 * HTTP communication, and XML response parsing for Homematic devices.
 */
class RpcUtil : public OpenKNX::Base
{
public:
    RpcUtil();
    const std::string name() override;

    // HTTP & XML Response handling
    bool sendRequestGetResponseDoc(arduino::String &request, tinyxml2::XMLDocument &doc);
    bool sendRequestCheckResponseOk(arduino::String &request);
    bool checkSendRequestResponse(tinyxml2::XMLDocument &doc);
    tinyxml2::XMLElement* getMethodResponseMember(tinyxml2::XMLDocument &doc);
    void debugLogResponse(HTTPClient &http, bool logResponse = false);
    
    // XML Request parameter builders
    void requestAddParamString(arduino::String &request, const char *str);
    void requestAddParamAddress(arduino::String &request, const char* deviceSerial, uint8_t channel);
    void requestAddParamDouble(arduino::String &request, double value);
    void requestAddParamInteger4(arduino::String &request, int32_t value);
    void requestAddParamBoolean(arduino::String &request, boolean value);

    // XML-RPC RPC value setting
    bool rpcSetValueDouble(const char* deviceSerial, const uint8_t channel, const char * paramName, double value);
    bool rpcSetValueBool(const char* deviceSerial, const uint8_t channel, const char * paramName, bool value);
    bool rpcSetValueInteger4(const char* deviceSerial, const uint8_t channel, const char * paramName, int32_t value);

private:
    const std::string logPrefix() { return "HMG<Client>"; }

};

extern RpcUtil hmgClient;
