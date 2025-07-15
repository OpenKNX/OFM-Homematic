// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#include "HomematicChannel.h"


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
            if (success)
            {
                updateRssi();
            }
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
        requestAddParamAddress(request, channels[i]);
        requestAddParamString(request, "VALUES");
        request += "</params>";
        request += "</methodCall>";

        tinyxml2::XMLDocument doc;
        success = sendRequestGetResponseDoc(request, doc) && updateKOsFromMethodResponse(doc, channels[i]);
    }
    logIndentDown();
    return success;
}

bool HomematicChannel::updateRssi()
{
    logDebugP("updateRssi()");

    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>rssiInfo</methodName>";
    request += "</methodCall>";

    tinyxml2::XMLDocument doc;
    return sendRequestGetResponseDoc(request, doc) && processRssiInfoResponse(doc);
}

tinyxml2::XMLElement* HomematicChannel::getMethodResponseMember(tinyxml2::XMLDocument &doc)
{
    // path in xml: //methodResponse/params/param/value/struct/member[]/value/$type

    tinyxml2::XMLElement *elem = doc.FirstChildElement("methodResponse");
    CHECK_NULL(elem, "/methodResponse")

    elem = elem->FirstChildElement("params");
    CHECK_NULL(elem, "/methodResponse/params")

    elem = elem->FirstChildElement("param");
    CHECK_NULL(elem, "/methodResponse/params/param")

    elem = elem->FirstChildElement("value");
    CHECK_NULL(elem, "/methodResponse/params/param/value")

    elem = elem->FirstChildElement("struct");
    CHECK_NULL(elem, "/methodResponse/params/param/value/struct")

    elem = elem->FirstChildElement("member");
    CHECK_NULL(elem, "/methodResponse/params/param/value/struct/member[]")

    return elem;
}

bool HomematicChannel::updateKOsFromMethodResponse(tinyxml2::XMLDocument &doc, const uint8_t channel)
{
    const uint32_t tStart = millis();

    tinyxml2::XMLElement *member = getMethodResponseMember(doc);
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
            const bool processed = processResponseParamDouble(channel, pName, value);
            logDebugP("%s @%d %24s(d)=%f", (processed ? "=>" : "//"), channel, pName, value);
        }
        else if (tinyxml2::XMLElement *i4Element = memberValue->FirstChildElement("i4"))
        {
            const int32_t value = i4Element->IntText();
            const bool processed = processResponseParamInt32(channel, pName, value);
            logDebugP("%s @%d %24s(i)=%d", (processed ? "=>" : "//"), channel, pName, value);
        }
        else if (tinyxml2::XMLElement *boolElement = memberValue->FirstChildElement("boolean"))
        {
            const bool value = boolElement->IntText();
            const bool processed = processResponseParamBool(channel, pName, value);
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

// TODO check moving to module OR replace with getParametSet for Channel :0
bool HomematicChannel::processRssiInfoResponse(tinyxml2::XMLDocument &doc)
{
    const uint32_t tStart = millis();

    tinyxml2::XMLElement *member = getMethodResponseMember(doc);
    if (member == nullptr)
    {
        return false;
    }

    for (/* init before*/; member != nullptr; member = member->NextSiblingElement("member"))
    {
        // structure:
        //   <member><name>$NAME</name><value><struct>..</struct></value></member>

        tinyxml2::XMLElement *elemNameSerial1 = member->FirstChildElement("name");
        CHECK_FALSE(elemNameSerial1, "../name") // /methodResponse/params/param/value/struct[]/member/name

        // TODO check moving after check of expected name
        tinyxml2::XMLElement *elemValue1 = member->FirstChildElement("value");
        CHECK_FALSE(elemValue1, "../value") // /methodResponse/params/param/value/struct[]/member/value

        // => <name> and <value> are present
        const char *serial1 = elemNameSerial1->GetText();
        if (strcmp(serial1, (const char *)ParamHMG_dDeviceSerial) == 0)
        {
            // => this is the device of current channel!

            tinyxml2::XMLElement *elemStruct = elemValue1->FirstChildElement("struct");
            CHECK_FALSE(elemStruct, "../value/struct")

            tinyxml2::XMLElement *elemMember2 = elemStruct->FirstChildElement("member");
            CHECK_FALSE(elemMember2, "../value/struct[]/member")

            tinyxml2::XMLElement *elemNameSerial2 = elemMember2->FirstChildElement("name");
            CHECK_FALSE(elemNameSerial2, "../value/struct[]/member/name")

            // TODO check moving after check of expected name
            tinyxml2::XMLElement *elemValue2 = elemMember2->FirstChildElement("value");
            CHECK_FALSE(elemValue2, "../value/struct[]/member/value")

            // => level2: <name> and <value> are present
            const char *serial2 = elemNameSerial2->GetText();
            if (true /*strcmp(serial2, (const char *)ParamHMG_dDeviceSerial) == 0*/)
            {

                tinyxml2::XMLElement *elemArray = elemValue2->FirstChildElement("array");
                CHECK_FALSE(elemArray, "../value/struct[]/member/value/array")

                tinyxml2::XMLElement *elemData = elemArray->FirstChildElement("data");
                CHECK_FALSE(elemData, "../value/struct[]/member/value/array/data")

                tinyxml2::XMLElement *elemValue1 = elemData->FirstChildElement("value");
                CHECK_FALSE(elemValue1, "../value/struct[]/member/value/array/data/value[0]")

                tinyxml2::XMLElement *elemValue2 = elemValue1->NextSiblingElement("value");
                CHECK_FALSE(elemValue2, "../value/struct[]/member/value/array/data/value[1]")

                tinyxml2::XMLElement *elemInt1 = elemValue1->FirstChildElement("i4");
                CHECK_FALSE(elemInt1, "../value/struct[]/member/value/array/data/value[0]/i4")

                tinyxml2::XMLElement *elemInt2 = elemValue2->FirstChildElement("i4");
                CHECK_FALSE(elemInt2, "../value/struct[]/member/value/array/data/value[1]/i4")

                const int32_t rssi1 = elemInt1->IntText();
                const int32_t rssi2 = elemInt2->IntText();

                logDebugP("RSSI: %s <-> %s", serial1, serial2);
                logIndentUp();

                if (rssi1 == 65536 || rssi2 == 65536)
                {
                    KoHMG_KOdSignalQuality.valueCompare((int32_t)0x7F, DPT_Value_2_Count);
                }
                else
                {
                    logDebugP("rssi1=%i / rssi2=%i", rssi1, rssi2);
                    KoHMG_KOdSignalQuality.valueCompare((rssi1 + rssi2)/2, DPT_Value_2_Count);
                }

                logIndentDown();

            }

        }

    }

    logDebugP("[DONE] processRssiInfoResponse() %d ms", millis() - tStart);
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
    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>setValue</methodName>";
    request += "<params>";
    requestAddParamAddress(request, channel);
    requestAddParamString(request, paramName);
    requestAddParamDouble(request, value);
    request += "</params>";
    request += "</methodCall>";

    logDebugP("XML-RPC call: setValue(%s, %s, %.3f)", ParamHMG_dDeviceSerial, paramName, value);

    return sendRequestCheckResponseOk(request);
}

bool HomematicChannel::rpcSetValueBool(const uint8_t channel, const char * paramName, bool value)
{
    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>setValue</methodName>";
    request += "<params>";
    requestAddParamAddress(request, channel);
    requestAddParamString(request, paramName);
    requestAddParamBoolean(request, value);
    request += "</params>";
    request += "</methodCall>";

    logDebugP("XML-RPC call: setValue(%s, %s, %s)", ParamHMG_dDeviceSerial, paramName, value ? "true" : "false");

    return sendRequestCheckResponseOk(request);
}

bool HomematicChannel::rpcSetValueInteger4(const uint8_t channel, const char * paramName, int32_t value)
{
    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>setValue</methodName>";
    request += "<params>";
    requestAddParamAddress(request, channel);
    requestAddParamString(request, paramName);
    requestAddParamInteger4(request, value);
    request += "</params>";
    request += "</methodCall>";

    logDebugP("XML-RPC call: setValue(%s, %s, %d)", ParamHMG_dDeviceSerial, paramName, value);

    return sendRequestCheckResponseOk(request);
}

void HomematicChannel::requestAddParamString(arduino::String &request, const char *str)
{
    request += "<param><value><string>";
    request += str;
    request += "</string></value></param>";
}

void HomematicChannel::requestAddParamAddress(arduino::String &request, uint8_t channel)
{
    request += "<param><value><string>";
    request += (const char *)ParamHMG_dDeviceSerial; //"OEQ1234567";
    request += ":";
    request += String(channel);
    request += "</string></value></param>";
}

void HomematicChannel::requestAddParamDouble(arduino::String &request, double targetTemperature)
{
    request += "<param><value><double>";
    request += targetTemperature;
    request += "</double></value></param>";
}

void HomematicChannel::requestAddParamInteger4(arduino::String &request, int32_t i4Value)
{
    request += "<param><value><i4>";
    request += i4Value;
    request += "</i4></value></param>";
}

void HomematicChannel::requestAddParamBoolean(arduino::String &request, boolean value)
{
    request += "<param><value><boolean>";
    request += value ? 1 : 0;
    request += "</boolean></value></param>";
}

bool HomematicChannel::sendRequestGetResponseDoc(arduino::String &request, tinyxml2::XMLDocument &doc)
{
    const uint32_t tStart = millis();

    logDebugP("Device Serial: %s", ParamHMG_dDeviceSerial);
    // logDebugP("Read URL POST: http://%s:%d", (const char *)ParamHMG_Host, ParamHMG_Port);

    HTTPClient http;
    /*
    // TODO check moving to module
    String url = "http://";
    url += (const char *)ParamHMG_Host;
    url += ":";
    url += ParamHMG_Port;

#ifdef ARDUINO_ARCH_RP2040
    if (url.startsWith("https://"))
        http.setInsecure();
#endif
    http.begin(url);
    */

    http.begin((const char *)ParamHMG_Host, ParamHMG_Port);
    // TODO check using reuse of the connection; needs moving to module
    http.setReuse(false);
    http.addHeader("Content-Type", "text/xml");
    http.addHeader("Accept", "text/xml");

    // send value read request
    int httpStatus = http.POST(request);
    if (httpStatus != 200)
    {
        http.end();
        logErrorP("POST request with status-code %d", httpStatus);
        return false;
    }

    logDebugP("[DONE] duration request %d ms", millis() - tStart);

    const uint32_t tStart2 = millis();
    debugLogResponse(http);

    const uint32_t tStart3 = millis();
    if (doc.Parse(http.getString().c_str()) != tinyxml2::XML_SUCCESS)
    {
        http.end();
        logErrorP("Parsing-Error, ID=%d", doc.ErrorID());
        return false;
    }
    logDebugP("[DONE] parse %d ms", millis() - tStart3);

    http.end();

    logDebugP("[DONE] sendRequest2 %d ms", millis() - tStart2);
    return true;
}

bool HomematicChannel::sendRequestCheckResponseOk(arduino::String &request)
{
    tinyxml2::XMLDocument doc;
    return sendRequestGetResponseDoc(request, doc) && checkSendRequestResponse(doc);
}

bool HomematicChannel::checkSendRequestResponse(tinyxml2::XMLDocument &doc)
{
    const uint32_t tStart = millis();

    /* OK:

    <?xml version="1.0" encoding="iso-8859-1"?>
    <methodResponse><params><param>
        <value></value>
    </param></params></methodResponse>
    */

    /* FAIL:

    <?xml version="1.0" encoding="iso-8859-1"?>
    <methodResponse><fault>
        <value><struct><member><name>faultCode</name><value><i4>-5</i4></value></member><member><name>faultString</name><value>Unknown parameter</value></member></struct></value>
    </fault></methodResponse>
    */

    tinyxml2::XMLElement *root = doc.FirstChildElement("methodResponse");
    CHECK_FALSE(root, "/methodResponse")

    if (/*tinyxml2::XMLElement *fault =*/ root->FirstChildElement("fault"))
    {
        // FAIL path in xml: //methodResponse/fault/value/struct/member[]/{name,value/$type}
        // TODO extract error-code!
        logErrorP("Failed! Element /methodResponse/fault/ is present!");
        // TODO check error-handling improvement
        return false;
    }

    tinyxml2::XMLElement *params = root->FirstChildElement("params");
    CHECK_FALSE(params, "/methodResponse/{fault,params}")

    tinyxml2::XMLElement *param = params->FirstChildElement("param");
    CHECK_FALSE(param, "/methodResponse/params/param")

    tinyxml2::XMLElement *value = param->FirstChildElement("value");
    CHECK_FALSE(value, "/methodResponse/params/param/value")

    logDebugP("[DONE] checkSendRequestResponse() in %d ms", millis() - tStart);
    return true;
}

void HomematicChannel::debugLogResponse(HTTPClient &http)
{
#ifdef OPENKNX_DEBUG
    if (_logResponse)
    {
        const uint32_t tStart2 = millis();

        String response = http.getString();
        logDebugP("response length: %d", response.length());
        const size_t len = response.length();
        const size_t lineLen = 100;
        for (size_t i = 0; i < len; i += lineLen)
        {
            logDebugP("response: %s", response.substring(i, std::min(i + lineLen, len)).c_str());
        }

        logDebugP("[DONE] duration log response %d ms", millis() - tStart2);
    }
#endif
}
