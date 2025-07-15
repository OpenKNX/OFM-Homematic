// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#include "HomematicChannelThermostat.h"

HomematicChannelThermostat::HomematicChannelThermostat(uint8_t index) 
    : HomematicChannel(index)
{
}

const std::string HomematicChannelThermostat::name()
{
    return "HMG-Thermostat";
}

bool HomematicChannelThermostat::processResponseParamDouble(uint8_t channel, const char* pName, double value)
{
    if (channel != 4)
    {
        return false; // ignore all other channels
    }
    else if (strcmp(pName, "ACTUAL_TEMPERATURE") == 0)
    {
        KoHMG_KOdTempCurrent.valueCompare(value, DPT_Value_Temp);
        return true;
    }
    else if (strcmp(pName, "BATTERY_STATE") == 0)
    {
        KoHMG_KOdBatteryVoltage.valueCompare(value * 1000, DPT_Value_Volt);
        return true;
    }
    else if (strcmp(pName, "SET_TEMPERATURE") == 0)
    {
        KoHMG_KOdTempSetCurrent.valueCompare(value, DPT_Value_Temp);
        return true;
    }
    return false;
}

bool HomematicChannelThermostat::processResponseParamInt32(uint8_t channel, const char* pName, int32_t value)
{
    if (channel != 4)
    {
        return false; // ignore all other channels
    }
    else if (strcmp(pName, "BOOST_STATE") == 0)
    {
        KoHMG_KOdBoostState.valueCompare(value, DPT_State);
        return true;
    }
    else if (strcmp(pName, "FAULT_REPORTING") == 0)
    {
        KoHMG_KOdError.valueCompare(value, DPT_Alarm);
        return true;
    }
    else if (strcmp(pName, "VALVE_STATE") == 0)
    {
        KoHMG_KOdValveState.valueCompare(value, DPT_Scaling);
        return true;
    }
    return false;
}

void HomematicChannelThermostat::processDeviceSpecificInputKo(GroupObject &ko)
{
    switch (HMG_KoCalcIndex(ko.asap()))
    {
        case HMG_KoKOdTempSet:
        {
            if (_allowedWriting)
            {
                const double temperature = KoHMG_KOdTempSet.value(DPT_Value_Temp);
                sendSetTemperature(temperature);
            }
            break;
        }
        case HMG_KoKOdBoostTrigger:
        {
            if (_allowedWriting)
            {
                sendBoost(KoHMG_KOdBoostTrigger.value(DPT_Trigger));
                updateRequestTiming(ParamHMG_RequestIntervallShort);
            }
            break;
        }
        default:
            // Unknown KO for this device type
            logTraceP("Unknown KO index for device type 1: %d", HMG_KoCalcIndex(ko.asap()));
            break;
    }
}

void HomematicChannelThermostat::sendSetTemperature(double targetTemperature)
{
    logDebugP("sendSetTemperature(%.3g)", targetTemperature);
    rpcSetValueDouble(getDeviceChannel(), "SET_TEMPERATURE", targetTemperature);
}

void HomematicChannelThermostat::sendBoost(bool boost)
{
    logDebugP("sendBoost(%s)", boost ? "true" : "false");
    rpcSetValueBool(getDeviceChannel(), "BOOST_MODE", boost);
}
