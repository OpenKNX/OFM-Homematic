// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2026 Cornelius Koepp

#pragma once
#include "HomematicChannel.h"

#define HMG_DEVTYPE__USER_DEFINED (7)
#define HMG_USERDEF_DATAPOINTS_COUNT (5)

#define HMG_ACCESS_MASK_READ (0x01)
#define HMG_ACCESS_MASK_WRITE (0x02)
#define HMG_ACCESS_MASK_EVENT (0x04)

// Makros for user-defined datapoints configuration parameters
// type:
#define HMG_dUD___TypeMask (HMG_dUD1TypeMask)
#define HMG_dUD___TypeShift (HMG_dUD1TypeShift)
static_assert(HMG_dUD1TypeMask == HMG_dUD___TypeMask); static_assert(HMG_dUD1TypeShift == HMG_dUD___TypeShift);
static_assert(HMG_dUD2TypeMask == HMG_dUD___TypeMask); static_assert(HMG_dUD2TypeShift == HMG_dUD___TypeShift);
static_assert(HMG_dUD3TypeMask == HMG_dUD___TypeMask); static_assert(HMG_dUD3TypeShift == HMG_dUD___TypeShift);
static_assert(HMG_dUD4TypeMask == HMG_dUD___TypeMask); static_assert(HMG_dUD4TypeShift == HMG_dUD___TypeShift);
static_assert(HMG_dUD5TypeMask == HMG_dUD___TypeMask); static_assert(HMG_dUD5TypeShift == HMG_dUD___TypeShift);
#define HMG_dUD___Type(IDX) (HMG_dUD1Type + IDX * (HMG_dUD2Type - HMG_dUD1Type))
static_assert(HMG_dUD___Type(0) == HMG_dUD1Type);
static_assert(HMG_dUD___Type(1) == HMG_dUD2Type);
static_assert(HMG_dUD___Type(2) == HMG_dUD3Type);
static_assert(HMG_dUD___Type(3) == HMG_dUD4Type);
static_assert(HMG_dUD___Type(4) == HMG_dUD5Type);
#define ParamHMG_dUD___Type(IDX) ((knx.paramByte(HMG_ParamCalcIndex(HMG_dUD___Type(IDX))) & HMG_dUD___TypeMask) >> HMG_dUD___TypeShift)
// access:
#define HMG_dUD___AccessMask (HMG_dUD1AccessMask)
#define HMG_dUD___AccessShift (HMG_dUD1AccessShift)
static_assert(HMG_dUD1AccessMask == HMG_dUD___AccessMask); static_assert(HMG_dUD1AccessShift == HMG_dUD___AccessShift);
static_assert(HMG_dUD2AccessMask == HMG_dUD___AccessMask); static_assert(HMG_dUD2AccessShift == HMG_dUD___AccessShift);
static_assert(HMG_dUD3AccessMask == HMG_dUD___AccessMask); static_assert(HMG_dUD3AccessShift == HMG_dUD___AccessShift);
static_assert(HMG_dUD4AccessMask == HMG_dUD___AccessMask); static_assert(HMG_dUD4AccessShift == HMG_dUD___AccessShift);
static_assert(HMG_dUD5AccessMask == HMG_dUD___AccessMask); static_assert(HMG_dUD5AccessShift == HMG_dUD___AccessShift);
#define HMG_dUD___Access(IDX) (HMG_dUD1Access + IDX * (HMG_dUD2Access - HMG_dUD1Access))
static_assert(HMG_dUD___Access(0) == HMG_dUD1Access);
static_assert(HMG_dUD___Access(1) == HMG_dUD2Access);
static_assert(HMG_dUD___Access(2) == HMG_dUD3Access);
static_assert(HMG_dUD___Access(3) == HMG_dUD4Access);
static_assert(HMG_dUD___Access(4) == HMG_dUD5Access);
#define ParamHMG_dUD___Access(IDX) ((knx.paramByte(HMG_ParamCalcIndex(HMG_dUD___Access(IDX))) & HMG_dUD___AccessMask) >> HMG_dUD___AccessShift)
// name:
/*
#define HMG_dUD___ParamNameMask (HMG_dUD1ParamNameMask)
#define HMG_dUD___ParamNameShift (HMG_dUD1ParamNameShift)
static_assert(HMG_dUD1ParamNameMask == HMG_dUD___ParamNameMask); static_assert(HMG_dUD1ParamNameShift == HMG_dUD___ParamNameShift);
static_assert(HMG_dUD2ParamNameMask == HMG_dUD___ParamNameMask); static_assert(HMG_dUD2ParamNameShift == HMG_dUD___ParamNameShift);
static_assert(HMG_dUD3ParamNameMask == HMG_dUD___ParamNameMask); static_assert(HMG_dUD3ParamNameShift == HMG_dUD___ParamNameShift);
static_assert(HMG_dUD4ParamNameMask == HMG_dUD___ParamNameMask); static_assert(HMG_dUD4ParamNameShift == HMG_dUD___ParamNameShift);
static_assert(HMG_dUD5ParamNameMask == HMG_dUD___ParamNameMask); static_assert(HMG_dUD5ParamNameShift == HMG_dUD___ParamNameShift);
*/
#define HMG_dUD___ParamName(IDX) (HMG_dUD1ParamName + IDX * (HMG_dUD2ParamName - HMG_dUD1ParamName))
static_assert(HMG_dUD___ParamName(0) == HMG_dUD1ParamName);
static_assert(HMG_dUD___ParamName(1) == HMG_dUD2ParamName);
static_assert(HMG_dUD___ParamName(2) == HMG_dUD3ParamName);
static_assert(HMG_dUD___ParamName(3) == HMG_dUD4ParamName);
static_assert(HMG_dUD___ParamName(4) == HMG_dUD5ParamName);
#define ParamHMG_dUD___ParamName(IDX) ((const char*)(knx.paramData(HMG_ParamCalcIndex(HMG_dUD___ParamName(IDX)))))
// KO:
#define HMG_KoKOdUD___Val(IDX) (HMG_KoKOdUD1Val + (IDX) * (HMG_KoKOdUD2Val - HMG_KoKOdUD1Val))
static_assert(HMG_KoKOdUD___Val(0) == HMG_KoKOdUD1Val);
static_assert(HMG_KoKOdUD___Val(1) == HMG_KoKOdUD2Val);
static_assert(HMG_KoKOdUD___Val(2) == HMG_KoKOdUD3Val);
static_assert(HMG_KoKOdUD___Val(3) == HMG_KoKOdUD4Val);
static_assert(HMG_KoKOdUD___Val(4) == HMG_KoKOdUD5Val);
#define KoHMG_KOdUD___Val(IDX) (knx.getGroupObject(HMG_KoCalcNumber(HMG_KoKOdUD___Val(IDX))))


/**
 * User-defined Homematic channel implementation for custom device configurations.
 * Supports up to 5 configurable datapoints with flexible data types and access patterns.
 */
class HomematicChannelUserDefined : public HomematicChannel
{
  private:

    // Helper methods for datapoint configuration
    const char* getDatapointParamName(uint8_t index) const;

    uint8_t _checkProcessResponseParam(const uint8_t datapointIndex, const uint8_t channel, const char* pName, const bool isEvent);
    
    // Helper method for processing individual datapoint KOs
    void processInputKo(const uint8_t datepointIndex, GroupObject &ko);
    
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
