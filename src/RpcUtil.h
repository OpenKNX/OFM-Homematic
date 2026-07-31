// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2026 Cornelius Koepp

#pragma once

#include <Arduino.h>
#include <tinyxml2.h>
#include <string>
#include "OpenKNX.h"
#include "OpenKNX/Base.h"
#include "OpenKNX/Network/Module.h" // openknxNetwork.webclient

#ifndef OPENKNX_WEBCLIENT
#error "OFM-Homematic requires the OFM-Network Webclient - define OPENKNX_WEBCLIENT in the device project"
#endif

#define ADDRESS_CHANNEL_NONE (0xff)

// Max size buffered from a single XML-RPC response (e.g. rssiInfo with many devices)
#ifndef HMG_RPC_MAX_RESPONSE_SIZE
#define HMG_RPC_MAX_RESPONSE_SIZE 8192
#endif

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
    bool sendRequestGetResponseDoc(String &request, tinyxml2::XMLDocument &doc);
    bool sendRequestCheckResponseOk(String &request);
    bool checkSendRequestResponse(tinyxml2::XMLDocument &doc);
    tinyxml2::XMLElement* getMethodResponseMember(tinyxml2::XMLDocument &doc);
    void debugLogResponse(const std::string response, bool logResponse = false);

    // XML Request parameter builders
    void requestAddParamString(String &request, const char *str);
    /**
     * Add an address as parameter to request. Channel will be excluded for `channel==ADDRESS_CHANNEL_NONE`.
     * @returns <param><value><string>{serial}[:{ch}]</string></value></param>
     */
    void requestAddParamAddress(String &request, const char* deviceSerial, uint8_t channel);
    void requestAddParamDouble(String &request, double value);
    void requestAddParamInteger4(String &request, int32_t value);
    void requestAddParamBoolean(String &request, boolean value);

    // XML-RPC RPC value setting
    bool rpcSetValueDouble(const char* deviceSerial, const uint8_t channel, const char * paramName, double value);
    bool rpcSetValueBool(const char* deviceSerial, const uint8_t channel, const char * paramName, bool value);
    bool rpcSetValueInteger4(const char* deviceSerial, const uint8_t channel, const char * paramName, int32_t value);

private:
    const std::string logPrefix() { return "HMG<Client>"; }

};

extern RpcUtil hmgClient;
