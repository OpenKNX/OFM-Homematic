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

uint8_t HomematicChannelUserDefined::getDeviceChannel() const
{
    return ParamHMG_dUDChannelNumber;
}

void HomematicChannelUserDefined::setup()
{
    HomematicChannel::setup();
}

void HomematicChannelUserDefined::processDeviceSpecificInputKo(GroupObject &ko)
{
    const uint8_t koIndex = HMG_KoCalcIndex(ko.asap());
    switch (koIndex) {
        case HMG_KoKOdUD1Set: processInputKo(0, ko); break;
        case HMG_KoKOdUD2Set: processInputKo(1, ko); break;
        case HMG_KoKOdUD3Set: processInputKo(2, ko); break;
        case HMG_KoKOdUD4Set: processInputKo(3, ko); break;
        case HMG_KoKOdUD5Set: processInputKo(4, ko); break;
        default:
            // ignore other KOs
            break;
    }
}

// Helper method for processing individual datapoint KOs
void HomematicChannelUserDefined::processInputKo(const uint8_t datepointIndex, GroupObject &ko)
{
    const uint8_t type = ParamHMG_dUD___Type(datepointIndex);
    // Check if datapoint is configured and writable
    if (type == 0) {
        logTraceP("Datapoint not configured (type = 0)");
        return;
    }

    const char* paramName = getDatapointParamName(datepointIndex);
    if ((ParamHMG_dUD___Access(datepointIndex) & HMG_ACCESS_MASK_WRITE) == 0) { // W bit (schreiben) not set
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

/**
 * Check if a value with a given name within a device channel should be processed be the datapoint definition.
 * @param datapointIndex - the index of the datapoint (0-4)
 * @param channel - the device channel number from which the parameter was received
 * @param pName - the parameter name string
 * @param isEvent - the reponse is from event, or reponse to read otherwise
 * @return the type of the datapoint if it should be processed, 0 otherwise
 */
uint8_t HomematicChannelUserDefined::_checkProcessResponseParam(const uint8_t datapointIndex, const uint8_t channel, const char* pName, const bool isEvent)
{
    if ((datapointIndex < HMG_USERDEF_DATAPOINTS_COUNT) && channel == getDeviceChannel())
    {
        const uint8_t type = ParamHMG_dUD___Type(datapointIndex);
        if ((type != 0)
            && (ParamHMG_dUD___Access(datapointIndex) & (isEvent ? HMG_ACCESS_MASK_EVENT : HMG_ACCESS_MASK_READ))
            && (strcmp(pName, getDatapointParamName(datapointIndex)) == 0))
        {
            return type;
        }
    }
    return 0;
}

bool HomematicChannelUserDefined::processResponseParamDouble(const uint8_t channel, const char* pName, const double value, const bool isEvent /*= false*/)
{
    // Find matching datapoint by parameter name
    for (int i = 0; i < HMG_USERDEF_DATAPOINTS_COUNT; i++) {
        const uint8_t type = _checkProcessResponseParam(i, channel, pName, isEvent);
        if (type)
        {
            switch (type)
            {
                case 3: // float type DPT9
                    if (setDatapointValue(i, value, DPT_Value_Tempd, isEvent)) {
                        logTraceP("Updated float datapoint %d (%s) with value: %f", i + 1, pName, value);
                        return true;
                    }
                    break;
                case 6: // float type DPT5.001
                    if (setDatapointValue(i, value * 100, DPT_Scaling, isEvent)) {
                        logTraceP("Updated float datapoint %d (%s) with value: %f", i + 1, pName, value);
                        return true;
                    }
                    break;
                case 8: // float type DPT14
                    if (setDatapointValue(i, value, DPT_Value_Amplitude, isEvent)) {
                        logTraceP("Updated float datapoint %d (%s) with value: %f", i + 1, pName, value);
                        return true;
                    }
                    break;
                default:
                    break; // ignore incompatible types
            }
        }
    }
    return false;
}

bool HomematicChannelUserDefined::processResponseParamInt32(const uint8_t channel, const char* pName, const int32_t value, const bool isEvent /*= false*/)
{
    // Find matching datapoint by parameter name
    for (int i = 0; i < HMG_USERDEF_DATAPOINTS_COUNT; i++) {
        const uint8_t type = _checkProcessResponseParam(i, channel, pName, isEvent);
        if (type)
        {
            // Set datapoint value based on type and log if successful
            bool success = false;
            switch (type) {
                case 4:
                case 5:
                    // integer or option type (DPT13)
                    success = setDatapointValue(i, value, DPT_Value_4_Count, isEvent);
                    break;
                case 7:
                    // integer or option type (DPT 5.001)
                    success = setDatapointValue(i, value, DPT_Scaling, isEvent);
                    break;
                case 9:
                case 10:
                    // integer or option type (DPT 5.005)
                    success = setDatapointValue(i, value, DPT_DecimalFactor, isEvent);
                    break;
                default:
                    break; // ignore incompatible types
            }
            
            if (success) {
                logTraceP("Updated integer datapoint %d (%s) with value: %d", i + 1, pName, value);
                return true;
            }
        }
    }
    return false;
}

bool HomematicChannelUserDefined::processResponseParamBool(const uint8_t channel, const char* pName, const bool value, const bool isEvent /*= false*/)
{
    // Find matching datapoint by parameter name
    for (int i = 0; i < HMG_USERDEF_DATAPOINTS_COUNT; i++) {
        const uint8_t type = _checkProcessResponseParam(i, channel, pName, isEvent);
        if (type)
        {
            switch (type) {
                case 1: // action type (DPT 1.017)
                case 2: // boolean type (DPT 1)
                    if (setDatapointValue(i, value, DPT_Switch, isEvent)) {
                        logTraceP("Updated bool datapoint %d (%s) with value: %s", i + 1, pName, value ? "true" : "false");
                        return true;
                    }
                    break;
                default:
                    break; // ignore incompatible types
            }
        }
    }
    return false;
}

// Helper methods using parameter macros

const char* HomematicChannelUserDefined::getDatapointParamName(uint8_t index) const
{
    return (index < HMG_USERDEF_DATAPOINTS_COUNT) ? (ParamHMG_dUD___ParamName(index)) : "";
}

// Helper to set GroupObject value with DPT
bool HomematicChannelUserDefined::setDatapointValue(uint8_t datapointIndex, const KNXValue& value, const Dpt& dpt, const bool byEvent /*= false*/)
{
    if (datapointIndex < HMG_USERDEF_DATAPOINTS_COUNT)
    {
        const bool forceSending = byEvent && ParamHMG_dUD___EventSend(datapointIndex);
        if (forceSending)
        {
            KoHMG_KOdUD___Val(datapointIndex).value(value, dpt);
        }
        else
        {
            KoHMG_KOdUD___Val(datapointIndex).valueCompare(value, dpt);
        }
        return true; // ignoring result of KO update. Return indicates the "usage" of KO...
    }
    else
    {
        return false;
    }
}
