// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

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
        logDebugP("Datapoint (%s) is not writable", paramName);
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
            logTraceP("Sent boolean value for %s: %s", paramName, value ? "true" : "false");
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
        case 5: // option (mapped to integer)
        {
            const int32_t value = (type != 7) ? ko.value(DPT_Value_4_Count) : ko.value(DPT_Scaling);
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
bool HomematicChannelUserDefined::processResponseParamDouble(uint8_t channel, const char* pName, double value)
{
    // Find matching datapoint by parameter name
    for (int i = 0; i < 5; i++) {
        if (isDatapointConfigured(i) && isDatapointReadable(i)) {
            const char* configuredName = getDatapointParamName(i);
            if (strcmp(pName, configuredName) == 0) {
                uint8_t type = getDatapointType(i);
                if (type == 3) { // float type DPT9
                    if (setDatapointValue(i, pName, value, DPT_Value_Tempd)) {
                        logTraceP("Updated float datapoint %d (%s) with value: %f", i + 1, pName, value);
                        return true;
                    }
                }
                else if (type == 6) { // float type DPT5.001
                    if (setDatapointValue(i, pName, value * 100, DPT_Scaling)) {
                        logTraceP("Updated float datapoint %d (%s) with value: %f", i + 1, pName, value);
                        return true;
                    }
                }
                else if (type == 8) { // float type DPT14
                    if (setDatapointValue(i, pName, value, DPT_Value_Amplitude)) {
                        logTraceP("Updated float datapoint %d (%s) with value: %f", i + 1, pName, value);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool HomematicChannelUserDefined::processResponseParamInt32(uint8_t channel, const char* pName, int32_t value)
{
    // Find matching datapoint by parameter name
    for (int i = 0; i < 5; i++) {
        if (isDatapointConfigured(i) && isDatapointReadable(i)) {
            const char* configuredName = getDatapointParamName(i);
            if (strcmp(pName, configuredName) == 0) {
                uint8_t type = getDatapointType(i);
                if (type == 4 || type == 5 || type == 7) { // integer or option type
                    if (setDatapointValue(i, pName, value, (type != 7) ? DPT_Value_4_Count : DPT_Scaling)) {
                        logTraceP("Updated integer datapoint %d (%s) with value: %d", i + 1, pName, value);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool HomematicChannelUserDefined::processResponseParamBool(uint8_t channel, const char* pName, bool value)
{
    // Find matching datapoint by parameter name
    for (int i = 0; i < 5; i++) {
        if (isDatapointConfigured(i) && isDatapointReadable(i)) {
            const char* configuredName = getDatapointParamName(i);
            if (strcmp(pName, configuredName) == 0) {
                uint8_t type = getDatapointType(i);
                if (type == 1 || type == 2) { // action or boolean type
                    if (setDatapointValue(i, pName, value, DPT_Switch)) {
                        logTraceP("Updated bool datapoint %d (%s) with value: %s", i + 1, pName, value ? "true" : "false");
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// Helper methods using parameter macros
bool HomematicChannelUserDefined::isDatapointConfigured(uint8_t index) const
{
    return (index < 5) && (getDatapointType(index) != 0);
}

uint8_t HomematicChannelUserDefined::getDatapointType(uint8_t index) const
{
    switch (index) {
        case 0: return ParamHMG_dUD1Type;
        case 1: return ParamHMG_dUD2Type;
        case 2: return ParamHMG_dUD3Type;
        case 3: return ParamHMG_dUD4Type;
        case 4: return ParamHMG_dUD5Type;
        default: return 0;
    }
}

// Helper method to get access parameter for datapoint
uint8_t HomematicChannelUserDefined::getDatapointAccess(uint8_t index) const
{
    switch (index) {
        case 0: return ParamHMG_dUD1Access;
        case 1: return ParamHMG_dUD2Access;
        case 2: return ParamHMG_dUD3Access;
        case 3: return ParamHMG_dUD4Access;
        case 4: return ParamHMG_dUD5Access;
        default: return 0;
    }
}

bool HomematicChannelUserDefined::isDatapointReadable(uint8_t index) const
{
    uint8_t access = getDatapointAccess(index);
    return access != 0 && (access & 0x01) != 0; // L bit (lesen)
}

bool HomematicChannelUserDefined::isDatapointWritable(uint8_t index) const
{
    uint8_t access = getDatapointAccess(index);
    return access != 0 && (access & 0x02) != 0; // W bit (schreiben)
}

bool HomematicChannelUserDefined::isDatapointEventBased(uint8_t index) const
{
    uint8_t access = getDatapointAccess(index);
    return access != 0 && (access & 0x04) != 0; // E bit (ereignisse)
}

const char*  HomematicChannelUserDefined::getDatapointParamName(uint8_t index) const
{
    switch (index) {
        case 0: return (const char*)ParamHMG_dUD1ParamName;
        case 1: return (const char*)ParamHMG_dUD2ParamName;
        case 2: return (const char*)ParamHMG_dUD3ParamName;
        case 3: return (const char*)ParamHMG_dUD4ParamName;
        case 4: return (const char*)ParamHMG_dUD5ParamName;
        default: return "";
    }
}

// Helper method to get KO index for datapoint
uint8_t HomematicChannelUserDefined::getDatapointKoIndex(uint8_t datapointIndex) const
{
    switch (datapointIndex) {
        case 0: return HMG_KoKOdUD1Val;
        case 1: return HMG_KoKOdUD2Val;
        case 2: return HMG_KoKOdUD3Val;
        case 3: return HMG_KoKOdUD4Val;
        case 4: return HMG_KoKOdUD5Val;
        default: return 0; // Invalid index
    }
}

// Helper to set GroupObject value with DPT
bool HomematicChannelUserDefined::setDatapointValue(uint8_t datapointIndex, const char* paramName, const KNXValue& value, const Dpt& dpt)
{
    uint8_t koIndex = getDatapointKoIndex(datapointIndex);
    if (koIndex == 0) return false;
    
    GroupObject& koObj = knx.getGroupObject(HMG_KoCalcNumber(koIndex));
    koObj.valueCompare(value, dpt);
    return true;
}
