// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#pragma once
#include "OpenKNX.h"

#include "HTTPClient.h"
#include <tinyxml2.h>


#define HMG_DEVTYPE__HM_CC_RT_DN__THERMOSTAT (1)
#define HMG_DEVTYPE__HM_LC_Sw1_Pl_DN_R1__SWITCH_ACTUATOR (6)
#define HMG_DEVTYPE__USER_DEFINED (7)


/**
 * Abstract base class for Homematic channels providing common functionality
 * for XML-RPC communication, parameter handling, and basic device management.
 */
class HomematicChannel : public OpenKNX::Channel
{
  private:
    bool _rssiDeviceFound = false;
    int32_t _rssiDeviceValue = 0;
    bool _rssiPeerFound = false;
    int32_t _rssiPeerValue = 0;

    bool _unreach = false;
    bool _batteryWarn = false;

    // handler-methods by value-type: first delegate to device-type, when not processed check :0
    bool _processResponseParamDouble(uint8_t channel, const char* pName, double value);
    bool _processResponseParamInt32(uint8_t channel, const char* pName, int32_t value);
    bool _processResponseParamBool(uint8_t channel, const char* pName, bool value);

  protected:
    // Channel state
    bool _channelActive = false;
    bool _running = false;
    bool _allowedWriting = true;

    // Timing
    uint32_t _lastRequest_millis = 0;
    uint32_t _requestInterval_millis = 3600 * 1000;

    // Common device operations
    bool update();
    void updateRequestTiming(uint16_t intervalInSeconds);
    
    // XML-RPC communication
    bool updateKOsFromMethodResponse(tinyxml2::XMLDocument &doc, const uint8_t channel);
    
    // XML-RPC RPC value setting
    bool rpcSetValueDouble(const uint8_t channel, const char * paramName, double value);
    bool rpcSetValueBool(const uint8_t channel, const char * paramName, bool value);
    bool rpcSetValueInteger4(const uint8_t channel, const char * paramName, int32_t value);

    // device-specific handler-methods by value-type
    /**
     * This method is called for double value parameters received in an XML-RPC response.
     * Derived classes should override this method.
     *
     * @param channel - the device channel number from which the parameter was received
     * @param pName - the parameter name string
     * @param value - the double value of the parameter
     * @return true if the parameter was successfully processed
     */
    virtual bool processResponseParamDouble(uint8_t channel, const char* pName, double value) { return false; }
    /**
     * @brief Processes a `i4` (32-bit integer) parameter from XML-RPC response
     *
     * This method is called for integer value parameters received in an XML-RPC response.
     * Derived classes should override this method.
     *
     * @param channel - the device channel number from which the parameter was received
     * @param pName - the parameter name string
     * @param value - the 32-bit integer value of the parameter
     * @return true if the parameter was successfully processed
     */
    virtual bool processResponseParamInt32(uint8_t channel, const char* pName, int32_t value) { return false; }
    /**
     * @brief Processes a `boolean` parameter from XML-RPC response
     *
     * This method is called for boolean value parameters received in an XML-RPC response.
     * Derived classes should override this method.
     *
     * @param channel - the device channel number from which the parameter was received
     * @param pName - the parameter name string
     * @param value - the boolean value of the parameter
     * @return true if the parameter was successfully processed
     */
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
