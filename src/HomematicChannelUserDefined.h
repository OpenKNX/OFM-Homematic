// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2026 Cornelius Koepp

#pragma once
#include "HomematicChannel.h"

#define HMG_DEVTYPE__USER_DEFINED (7)
#define HMG_USERDEF_DATAPOINTS_COUNT (5)

#define HMG_ACCESS_MASK_READ (0x01)
#define HMG_ACCESS_MASK_WRITE (0x02)
#define HMG_ACCESS_MASK_EVENT (0x04)

/**
 * User-defined Homematic channel implementation for custom device configurations.
 * Supports up to 5 configurable datapoints with flexible data types and access patterns.
 */
class HomematicChannelUserDefined : public HomematicChannel
{
  private:
    uint8_t _datapointType[5];
    uint8_t _datapointAccess[5];

    // Helper methods for datapoint configuration
    const char* getDatapointParamName(uint8_t index) const;

    uint8_t _checkProcessResponseParam(const uint8_t datapointIndex, const uint8_t channel, const char* pName, const bool isEvent);
    
    // Helper method for processing individual datapoint KOs
    void processInputKo(uint8_t access, uint8_t type, const uint32_t posParamName, GroupObject &ko);
    
    // Helper method to get access parameter for datapoint
    uint8_t getDatapointAccess(uint8_t index) const;
    
    // Helper to set GroupObject value with DPT
    bool setDatapointValue(uint8_t datapointIndex, const char* paramName, const KNXValue& value, const Dpt& dpt);

  protected:
    uint8_t getDeviceChannel() const override;

  public:
    HomematicChannelUserDefined(uint8_t index);
    virtual ~HomematicChannelUserDefined() = default;

    const std::string name() override;
    void setup() override;
    
    // Device-specific implementations
    void processDeviceSpecificInputKo(GroupObject &ko) override;
    
    // Response parameter processing for reading
    bool processResponseParamDouble(const uint8_t channel, const char* pName, const double value, const bool isEvent = false) override;
    bool processResponseParamInt32(const uint8_t channel, const char* pName, const int32_t value, const bool isEvent = false) override;
    bool processResponseParamBool(const uint8_t channel, const char* pName, const bool value, const bool isEvent = false) override;
};
