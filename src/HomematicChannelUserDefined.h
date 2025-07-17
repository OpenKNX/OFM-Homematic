// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#pragma once
#include "HomematicChannel.h"

#define HMG_DEVTYPE__USER_DEFINED (7)

/**
 * User-defined Homematic channel implementation for custom device configurations.
 * Supports up to 5 configurable datapoints with flexible data types and access patterns.
 */
class HomematicChannelUserDefined : public HomematicChannel
{
  private:
    // Helper methods for datapoint configuration
    bool isDatapointConfigured(uint8_t index) const;
    bool isDatapointReadable(uint8_t index) const;
    bool isDatapointWritable(uint8_t index) const;
    bool isDatapointEventBased(uint8_t index) const;
    uint8_t getDatapointType(uint8_t index) const;
    const char* getDatapointParamName(uint8_t index) const;
    
    // Helper method for processing individual datapoint KOs
    void processInputKo(uint8_t access, uint8_t type, const uint32_t posParamName, GroupObject &ko);
    
    // Helper method to get KO index for datapoint
    uint8_t getDatapointKoIndex(uint8_t datapointIndex) const;
    
    // Helper method to get access parameter for datapoint
    uint8_t getDatapointAccess(uint8_t index) const;
    
    // Helper to set GroupObject value with DPT
    bool setDatapointValue(uint8_t datapointIndex, const char* paramName, const KNXValue& value, const Dpt& dpt);

  public:
    HomematicChannelUserDefined(uint8_t index);
    virtual ~HomematicChannelUserDefined() = default;

    const std::string name() override;
    
    // Device-specific implementations
    void processDeviceSpecificInputKo(GroupObject &ko) override;
    uint8_t getDeviceChannel() const override { return ParamHMG_dUDChannelNumber; }
    
    // Response parameter processing for reading
    bool processResponseParamDouble(uint8_t channel, const char* pName, double value) override;
    bool processResponseParamInt32(uint8_t channel, const char* pName, int32_t value) override;
    bool processResponseParamBool(uint8_t channel, const char* pName, bool value) override;
};
