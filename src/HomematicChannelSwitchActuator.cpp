// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#include "HomematicChannelSwitchActuator.h"

HomematicChannelSwitchActuator::HomematicChannelSwitchActuator(uint8_t index) 
    : HomematicChannel(index)
{
}

const std::string HomematicChannelSwitchActuator::name()
{
    return "HMG-SwitchActuator";
}

bool HomematicChannelSwitchActuator::processResponseParamBool(uint8_t channel, const char* pName, bool value)
{
    if (channel != 1)
    {
        return false; // ignore all other channels
    }
    else if (strcmp(pName, "INHIBIT") == 0)
    {
        KoHMG_KOdLockState.valueCompare(value, DPT_State);
        return true;
    }
    else if (strcmp(pName, "STATE") == 0)
    {
        KoHMG_KOdSwitchState.valueCompare(value, DPT_Switch);
        return true;
    }
    else if (strcmp(pName, "WORKING") == 0)
    {
        // TODO KO: create and define output type!
        // KoHMG_KOdWorking.valueCompare(value, DPT_State);
        return true;
    }
    return false;
}

void HomematicChannelSwitchActuator::processDeviceSpecificInputKo(GroupObject &ko)
{
    switch (HMG_KoCalcIndex(ko.asap()))
    {
        case HMG_KoKOdSwitch:
        {
            if (_allowedWriting)
            {
                rpcSetValueBool(getDeviceChannel(), "STATE", KoHMG_KOdSwitch.value(DPT_Switch));
                updateRequestTiming(ParamHMG_RequestIntervallShort);
            }
            break;
        }
        case HMG_KoKOdSwitchStair:
        {
            if (_allowedWriting)
            {
                sendSwitchStairlight(KoHMG_KOdSwitchStair.value(DPT_Switch));
            }
            break;
        }
        case HMG_KoKOdLock:
        {
            if (_allowedWriting)
            {
                rpcSetValueBool(getDeviceChannel(), "INHIBIT", KoHMG_KOdLock.value(DPT_Switch));
                updateRequestTiming(ParamHMG_RequestIntervallShort);
            }
            break;
        }
        default:
            // Unknown KO for this device type
            logTraceP("Unknown KO index for device type 6: %d", HMG_KoCalcIndex(ko.asap()));
            break;
    }
}

void HomematicChannelSwitchActuator::sendSwitchStairlight(bool switchStair)
{
    if (switchStair)
    {
        // 1. set the on-time (for switching on)
        const double onTime_s = ParamHMG_dOnTimeTimeMS / 1000.0;
        rpcSetValueDouble(getDeviceChannel(), "ON_TIME", onTime_s);
    }
    if (switchStair || true /* allow switch off */)
    {
        // 2. switch on/off
        rpcSetValueBool(getDeviceChannel(), "STATE", switchStair);
        updateRequestTiming(ParamHMG_RequestIntervallShort);
    }
}
