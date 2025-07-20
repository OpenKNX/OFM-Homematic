// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#include "HomematicChannel.h"
#include "HomematicModule.h"
#include "RpcUtil.h"


HomematicChannel::HomematicChannel(uint8_t index)
{
    _channelIndex = index;
    // do not request all at the same time

    _requestInterval_millis = ParamHMG_RequestIntervall * 1000;
    // TODO cleanup based on channel
    _lastRequest_millis = 2000 * _channelIndex;
}

const std::string HomematicChannel::name()
{
    return "Homematic-Channel";
}

void HomematicChannel::setup()
{
    _channelActive = (ParamHMG_dDeviceType != 0) && !ParamHMG_dDisable;
    if (_channelActive)
    {
        _allowedWriting = ParamHMG_dWrite;
        logDebugP("active (Serial=%s)", ParamHMG_dDeviceSerial);
        // logDebugP("active (write=%u; serial='%s')", _allowedWriting, ParamHMG_dDeviceSerial);
    }
}

void HomematicChannel::processAfterStartupDelay()
{
    // start after device global delay
    if (_channelActive)
    {
        logDebugP("processAfterStartupDelay");
        _running = true;
    }
}

void HomematicChannel::loop()
{
    // !_channelActive will result in _running=false, so no need for checking
    if (_running)
    {
        if (delayCheckMillis(_lastRequest_millis, _requestInterval_millis))
        {
            const bool success = update();
            if (KoHMG_KOdReachable.valueNoSendCompare(success, DPT_Switch))
            {
                KoHMG_KOdReachable.objectWritten();
            }
            _requestInterval_millis = ParamHMG_RequestIntervall * 1000;
            _lastRequest_millis = millis();
        }
    }
}

bool HomematicChannel::update()
{
    logDebugP("update()");
    logIndentUp();

    const uint8_t channel = getDeviceChannel();

    // Process channels 0 and current channel
    const uint8_t channels[] = {0, channel};
    const uint8_t numChannels = (channel == 0) ? 1 : 2;

    bool success = true;
    for (uint8_t i = 0; (i < numChannels) && success; i++)
    {
        logDebugP("getParamset('%s:%u', VALUES)", (const char *)ParamHMG_dDeviceSerial, channels[i]);
        String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
        request += "<methodCall>";
        request += "<methodName>getParamset</methodName>";
        request += "<params>";
        hmgClient.requestAddParamAddress(request, (const char *)ParamHMG_dDeviceSerial, channels[i]);
        hmgClient.requestAddParamString(request, "VALUES");
        request += "</params>";
        request += "</methodCall>";

        tinyxml2::XMLDocument doc;
        success = hmgClient.sendRequestGetResponseDoc(request, doc) && updateKOsFromMethodResponse(doc, channels[i]);
    }
    openknxHomematicModule.updateDeviceStates(_channelIndex, _unreach, _batteryWarn, !success);
    logIndentDown();
    return success;
}

bool HomematicChannel::updateKOsFromMethodResponse(tinyxml2::XMLDocument &doc, const uint8_t channel)
{
    const uint32_t tStart = millis();

    tinyxml2::XMLElement *member = hmgClient.getMethodResponseMember(doc);
    if (member == nullptr)
    {
        return false;
    }

    for (/* init before*/; member != nullptr; member = member->NextSiblingElement("member"))
    {
        // structure:
        //   <member><name>$NAME</name><value><$TYPE>$VALUE</$TYPE></value></member>
        tinyxml2::XMLElement *memberName = member->FirstChildElement("name");
        tinyxml2::XMLElement *memberValue = member->FirstChildElement("value");
        CHECK_FALSE(memberName, "/methodResponse/params/param/value/struct[]/member/name")
        CHECK_FALSE(memberValue, "/methodResponse/params/param/value/struct[]/member/value")

        // => <name> and <value> are present
        const char *pName = memberName->GetText();
        if (tinyxml2::XMLElement *doubleElement = memberValue->FirstChildElement("double"))
        {
            const double value = doubleElement->DoubleText();
            const bool processed = _processResponseParamDouble(channel, pName, value);
            logDebugP("%s @%d %24s(d)=%f", (processed ? "=>" : "//"), channel, pName, value);
        }
        else if (tinyxml2::XMLElement *i4Element = memberValue->FirstChildElement("i4"))
        {
            const int32_t value = i4Element->IntText();
            const bool processed = _processResponseParamInt32(channel, pName, value);
            logDebugP("%s @%d %24s(i)=%d", (processed ? "=>" : "//"), channel, pName, value);
        }
        else if (tinyxml2::XMLElement *boolElement = memberValue->FirstChildElement("boolean"))
        {
            const bool value = boolElement->IntText();
            const bool processed = _processResponseParamBool(channel, pName, value);
            logDebugP("%s @%d %24s(b)=%d", (processed ? "=>" : "//"), channel, pName, value);
        }
        else
        {
            logDebugP("// @%d %24s(other ignored)", channel, pName);
        }
    }

    logDebugP("[DONE] updateKOsFromMethodResponse() %d ms", millis() - tStart);
    return true;
}

void HomematicChannel::processInputKo(GroupObject &ko)
{
    if (!_channelActive)
        return; // ignore inactive channel

    // Common KOs for all device types
    if (HMG_KoCalcIndex(ko.asap()) == HMG_KoKOdTriggerRequest)
    {
            if (KoHMG_KOdTriggerRequest.value(DPT_Trigger))
            {
                const bool success = update();
                KoHMG_KOdReachable.value(success, DPT_Switch);
                updateRequestTiming(ParamHMG_RequestIntervall);
            }
    }
    else
    {
        // Delegate to device-specific implementation
        processDeviceSpecificInputKo(ko);
    }
}

// Helper method to update request timing
inline void HomematicChannel::updateRequestTiming(uint16_t intervalInSeconds)
{
    _requestInterval_millis = intervalInSeconds * 1000;
    _lastRequest_millis = millis();
}

bool HomematicChannel::processCommandOverview()
{
    return false;
}

bool HomematicChannel::rpcSetValueDouble(const uint8_t channel, const char * paramName, double value)
{
    return hmgClient.rpcSetValueDouble((const char *)ParamHMG_dDeviceSerial, channel, paramName, value);
}

bool HomematicChannel::rpcSetValueBool(const uint8_t channel, const char * paramName, bool value)
{
    return hmgClient.rpcSetValueBool((const char *)ParamHMG_dDeviceSerial, channel, paramName, value);
}

bool HomematicChannel::rpcSetValueInteger4(const uint8_t channel, const char * paramName, int32_t value)
{
    return hmgClient.rpcSetValueInteger4((const char *)ParamHMG_dDeviceSerial, channel, paramName, value);
}

// Channel :0 parameter handlers (device-level parameters)
bool HomematicChannel::_processResponseParamDouble(uint8_t channel, const char* pName, double value)
{
    if (processResponseParamDouble(channel, pName, value))
    {
        return true; // processed by device-type
    }
    if (channel != 0)
    {
        return false; // not in :0
    }

    // Currently no double parameters known for Channel :0
    // Placeholder for future use
    return false;
}

bool HomematicChannel::_processResponseParamInt32(uint8_t channel, const char* pName, int32_t value)
{
    if (processResponseParamInt32(channel, pName, value))
    {
        return true; // processed by device-type
    }
    bool processed = false;
    if (channel != 0)
    {
        return false; // not in :0
    }
    else if (strcmp(pName, "RSSI_DEVICE") == 0)
    {
        _rssiDeviceFound = true;
        _rssiDeviceValue = value;
        processed = true; // return true;
    }
    else if (strcmp(pName, "RSSI_PEER") == 0)
    {
        _rssiPeerFound = true;
        _rssiPeerValue = value;
        processed = true; // return true;
    }
    /*
    else if (strcmp(pName, "AES_KEY") == 0)
    {
        return false; // TODO check implementation
    }
    */

    // update signal quality when BOTH values were found
    if (_rssiDeviceFound && _rssiPeerFound)
    {
        const int32_t worst_rssi = min(_rssiDeviceValue, _rssiPeerValue);
        // Linear mapping: -30dBm=100%, -90dBm=0%
        const int32_t quality_percent = max(0, min(100, (worst_rssi + 90) * 100 / 60));

        logDebugP("RSSI_DEVICE=%d, RSSI_PEER=%d => %d%%", _rssiDeviceValue, _rssiPeerValue, quality_percent);

        if (_rssiDeviceValue == -65535 || _rssiPeerValue == -65535)
        {
            KoHMG_KOdSignalQuality.valueCompare((int32_t)0x7F, DPT_Value_2_Count);
        }
        else
        {
            KoHMG_KOdSignalQuality.valueCompare(worst_rssi, DPT_Value_2_Count);
        }

        // TODO reset before start
        _rssiDeviceFound = false;
        _rssiPeerFound = false;
        return true;
    }

    return processed;
}

bool HomematicChannel::_processResponseParamBool(uint8_t channel, const char* pName, bool value)
{
    if (processResponseParamBool(channel, pName, value))
    {
        return true; // processed by device-type
    }
    if (channel != 0)
    {
        return false; // not in :0
    }
    else if (strcmp(pName, "CONFIG_PENDING") == 0)
    {
        return false; // TODO check implementation
    }
    else if (strcmp(pName, "DEVICE_IN_BOOTLOADER") == 0)
    {
        return false; // TODO check implementation
    }
    else if (strcmp(pName, "INHIBIT") == 0)
    {
        return false; // TODO check implementation
    }
    else if (strcmp(pName, "LOWBAT") == 0)
    {
        _batteryWarn = value;
        return true;
    }
    else if (strcmp(pName, "STICKY_UNREACH") == 0)
    {
        return false; // TODO check implementation
    }
    else if (strcmp(pName, "UNREACH") == 0)
    {
        _unreach = value;
        return true;
    }
    else if (strcmp(pName, "UPDATE_PENDING") == 0)
    {
        return false; // TODO check implementation
    }
    return false;
}
