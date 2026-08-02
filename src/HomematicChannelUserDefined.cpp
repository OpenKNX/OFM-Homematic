// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2026 Cornelius Koepp

#include "HomematicChannelUserDefined.h"
#include "HomematicModule.h"

HomematicChannelUserDefined::HomematicChannelUserDefined(uint8_t index)
    : HomematicChannel(index)
{
}

const std::string HomematicChannelUserDefined::name()
{
    return "HMG-UserDefined";
}

void HomematicChannelUserDefined::setup()
{
    HomematicChannel::setup();
    _datapointType[0] = ParamHMG_dUD1Type;
    _datapointType[1] = ParamHMG_dUD2Type;
    _datapointType[2] = ParamHMG_dUD3Type;
    _datapointType[3] = ParamHMG_dUD4Type;
    _datapointType[4] = ParamHMG_dUD5Type;
    _datapointAccess[0] = ParamHMG_dUD1Access;
    _datapointAccess[1] = ParamHMG_dUD2Access;
    _datapointAccess[2] = ParamHMG_dUD3Access;
    _datapointAccess[3] = ParamHMG_dUD4Access;
    _datapointAccess[4] = ParamHMG_dUD5Access;
}

void HomematicChannelUserDefined::processDeviceSpecificInputKo(GroupObject &ko)
{
    uint8_t koIndex = HMG_KoCalcIndex(ko.asap());
    switch (koIndex) {
        case HMG_KoKOdUD1Set: processInputKo(ParamHMG_dUD1Access, ParamHMG_dUD1Type, HMG_dUD1ParamName, ko); break;
        case HMG_KoKOdUD2Set: processInputKo(ParamHMG_dUD2Access, ParamHMG_dUD2Type, HMG_dUD2ParamName, ko); break;
        case HMG_KoKOdUD3Set: processInputKo(ParamHMG_dUD3Access, ParamHMG_dUD3Type, HMG_dUD3ParamName, ko); break;
        case HMG_KoKOdUD4Set: processInputKo(ParamHMG_dUD4Access, ParamHMG_dUD4Type, HMG_dUD4ParamName, ko); break;
        case HMG_KoKOdUD5Set: processInputKo(ParamHMG_dUD5Access, ParamHMG_dUD5Type, HMG_dUD5ParamName, ko); break;
        default:
            // ignore other KOs
            break;
    }
}

// Helper method for processing individual datapoint KOs
void HomematicChannelUserDefined::processInputKo(uint8_t access, uint8_t type, const uint32_t posParamName, GroupObject &ko)
{
    // Check if datapoint is configured and writable
    if (type == 0) {
        logTraceP("Datapoint not configured (type = 0)");
        return;
    }

    const char* paramName = (const char*)knx.paramData(HMG_ParamCalcIndex(posParamName));
    if ((access & 0x02) == 0) { // W bit (schreiben) not set
        logTraceP("Datapoint (%s) is not writable", paramName);
        return;
    }
    
    // Convert KO value to XML-RPC parameter based on type
    switch (type) {
        case 1: // action (DPT 1.017 - Trigger)
            if (ko.value(DPT_Trigger))
            {
                rpcSetValueBool(getDeviceChannel(), paramName, true);
                logTraceP("Sent action trigger for %s", paramName);
            }
            break;
            
        case 2: // boolean (DPT 1)
        {
            const bool value = ko.value(DPT_Switch);
            rpcSetValueBool(getDeviceChannel(), paramName, value);
            logTraceP("Sent boolean value for %s: %d", paramName, value);
            break;
        }
        case 3: // float (DPT 9)
        case 6: // float as percent (DPT 5.001)
        case 8: // float (DPT 14)
        {
            const double value = (type == 3) 
                ? (double)ko.value(DPT_Value_Tempd) 
                : (type == 8 
                    ? (double)ko.value(DPT_Value_Amplitude) 
                    : ((double)ko.value(DPT_Scaling)) / 100.0
                );
            rpcSetValueDouble(getDeviceChannel(), paramName, value);
            logTraceP("Sent float value for %s: %f", paramName, value);
            break;
        }
        case 4: // integer (DPT 13)
        case 7: // integer as percent (DPT5.001)
        case 5: // option (mapped to integer / DPT 13)
        case 9: // integer as decimal factor (DPT 5.005)
        case 10: // integer as decimal factor (DPT 5.005)
        {
            const int32_t value = (type == 7)
                ? ko.value(DPT_Scaling)
                : (
                    (type == 4 || type == 5) 
                        ? ko.value(DPT_Value_4_Count) 
                        : ko.value(DPT_DecimalFactor)
                );
            rpcSetValueInteger4(getDeviceChannel(), paramName, value);
            logTraceP("Sent integer value for %s: %d", paramName, value);
            break;
        }
        default:
            logDebugP("Unsupported datapoint type %d for parameter %s", type, paramName);
            break;
    }
}

// Response parameter processing for reading values from CCU
bool HomematicChannelUserDefined::processResponseParamDouble(const uint8_t channel, const char* pName, const double value, const bool isEvent /*= false*/)
{
    // Find matching datapoint by parameter name
    for (int i = 0; i < 5; i++) {
        if (isDatapointConfigured(i) && isDatapointReadable(i)) {
            const char* configuredName = getDatapointParamName(i);
            if (strcmp(pName, configuredName) == 0) {
                uint8_t type = _datapointType[i];
                switch (type)
                {
                    case 3: // float type DPT9
                        if (setDatapointValue(i, pName, value, DPT_Value_Tempd)) {
                            logTraceP("Updated float datapoint %d (%s) with value: %f", i + 1, pName, value);
                            return true;
                        }
                        break;
                    case 6: // float type DPT5.001
                        if (setDatapointValue(i, pName, value * 100, DPT_Scaling)) {
                            logTraceP("Updated float datapoint %d (%s) with value: %f", i + 1, pName, value);
                            return true;
                        }
                        break;
                    case 8: // float type DPT14
                        if (setDatapointValue(i, pName, value, DPT_Value_Amplitude)) {
                            logTraceP("Updated float datapoint %d (%s) with value: %f", i + 1, pName, value);
                            return true;
                        }
                        break;
                }
            }
        }
    }
    return false;
}

bool HomematicChannelUserDefined::processResponseParamInt32(const uint8_t channel, const char* pName, const int32_t value, const bool isEvent /*= false*/)
{
    // Find matching datapoint by parameter name
    for (int i = 0; i < 5; i++) {
        if (isDatapointConfigured(i) && isDatapointReadable(i)) {
            const char* configuredName = getDatapointParamName(i);
            if (strcmp(pName, configuredName) == 0) {
                uint8_t type = _datapointType[i];
                
                // Set datapoint value based on type and log if successful
                bool success = false;
                switch (type) {
                    case 4:
                    case 5:
                        // integer or option type (DPT13)
                        success = setDatapointValue(i, pName, value, DPT_Value_4_Count);
                        break;
                    case 7:
                        // integer or option type (DPT 5.001)
                        success = setDatapointValue(i, pName, value, DPT_Scaling);
                        break;
                    case 9:
                    case 10:
                        // integer or option type (DPT 5.005)
                        success = setDatapointValue(i, pName, value, DPT_DecimalFactor);
                        break;
                    default:
                        continue; // Skip unsupported types
                }
                
                if (success) {
                    logTraceP("Updated integer datapoint %d (%s) with value: %d", i + 1, pName, value);
                    return true;
                }
            }
        }
    }
    return false;
}

bool HomematicChannelUserDefined::processResponseParamBool(const uint8_t channel, const char* pName, const bool value, const bool isEvent /*= false*/)
{
    // Find matching datapoint by parameter name
    for (int i = 0; i < 5; i++) {
        const uint8_t type = _datapointType[i]; // TODO used in isDatapointConfigured
        if (isDatapointConfigured(i) && isDatapointReadable(i)) {
            const char* configuredName = getDatapointParamName(i);
            if (strcmp(pName, configuredName) == 0) {
                switch (type) {
                    case 1: // action type (DPT 1.017)
                    case 2: // boolean type (DPT 1)
                        if (setDatapointValue(i, pName, value, DPT_Switch)) {
                            logTraceP("Updated bool datapoint %d (%s) with value: %s", i + 1, pName, value ? "true" : "false");
                            return true;
                        }
                        break;
                }
            }
        }
    }
    return false;
}

// Helper methods using parameter macros
bool HomematicChannelUserDefined::isDatapointConfigured(uint8_t index) const
{
    return (index < 5) && (_datapointType[index] != 0);
}

bool HomematicChannelUserDefined::isDatapointReadable(uint8_t index) const
{
    return (index < 5) && (_datapointAccess[index] & 0x01); // L bit (lesen)
}

bool HomematicChannelUserDefined::isDatapointWritable(uint8_t index) const
{
    return (index < 5) && (_datapointAccess[index] & 0x02); // W bit (schreiben)
}

bool HomematicChannelUserDefined::isDatapointEventBased(uint8_t index) const
{
    return (index < 5) && (_datapointAccess[index] & 0x04); // E bit (ereignisse)
}

const char* HomematicChannelUserDefined::getDatapointParamName(uint8_t index) const
{
    #define HMG_dUD_ParamName_Distance (HMG_dUD2ParamName - HMG_dUD1ParamName)
    static_assert(HMG_dUD2ParamName == HMG_dUD1ParamName + HMG_dUD_ParamName_Distance, "User Defined Datapoint: Param Name Distance Missmatch between 1 and 2");
    static_assert(HMG_dUD3ParamName == HMG_dUD2ParamName + HMG_dUD_ParamName_Distance, "User Defined Datapoint: Param Name Distance Missmatch between 2 and 3");
    static_assert(HMG_dUD4ParamName == HMG_dUD3ParamName + HMG_dUD_ParamName_Distance, "User Defined Datapoint: Param Name Distance Missmatch between 3 and 4");
    static_assert(HMG_dUD5ParamName == HMG_dUD4ParamName + HMG_dUD_ParamName_Distance, "User Defined Datapoint: Param Name Distance Missmatch between 4 and 5");

    return (index < 5) ? (const char*)(knx.paramData(HMG_ParamCalcIndex(HMG_dUD1ParamName + index * HMG_dUD_ParamName_Distance))) : "";
}

// Helper to set GroupObject value with DPT
bool HomematicChannelUserDefined::setDatapointValue(uint8_t datapointIndex, const char* paramName, const KNXValue& value, const Dpt& dpt)
{
    if (datapointIndex < 5)
    {
        #define HMG_KoKOdUD_Val_Distance (HMG_KoKOdUD2Val - HMG_KoKOdUD1Val)
        static_assert(HMG_KoKOdUD2Val ==  HMG_KoKOdUD1Val + HMG_KoKOdUD_Val_Distance, "User Defined Datapoint: Value Distance Missmatch between 1 and 2");
        static_assert(HMG_KoKOdUD3Val ==  HMG_KoKOdUD2Val + HMG_KoKOdUD_Val_Distance, "User Defined Datapoint: Value Distance Missmatch between 2 and 3");
        static_assert(HMG_KoKOdUD4Val ==  HMG_KoKOdUD3Val + HMG_KoKOdUD_Val_Distance, "User Defined Datapoint: Value Distance Missmatch between 3 and 4");
        static_assert(HMG_KoKOdUD5Val ==  HMG_KoKOdUD4Val + HMG_KoKOdUD_Val_Distance, "User Defined Datapoint: Value Distance Missmatch between 4 and 5");

        GroupObject& koObj = knx.getGroupObject(HMG_KoCalcNumber(HMG_KoKOdUD1Val + datapointIndex * HMG_KoKOdUD_Val_Distance));
        koObj.valueCompare(value, dpt);
        return true;
    }
    else
    {
        return false;
    }
}
