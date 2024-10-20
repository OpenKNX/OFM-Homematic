// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024 Cornelius Koepp

#include "HomematicChannel.h"


#define CHECK_NULL(element, name) \
    if (element == nullptr) { \
        logErrorP("Element " name " is missing!"); \
        return nullptr; \
    }

#define CHECK_FALSE(element, name) \
    if (element == nullptr) { \
        logErrorP("Element " name " is missing!"); \
        return false; \
    }


HomematicChannel::HomematicChannel(uint8_t index)
{
    _channelIndex = index;
    // do not request all at the same time
    
    // TODO cleanup based on channel
    _lastRequest_millis = 1000 * _channelIndex;
}

const std::string HomematicChannel::name()
{
    return "Homematic-Channel";
}

void HomematicChannel::setup()
{
    _channelActive = (ParamHMG_dActive == 0b01);
    if (_channelActive)
    {
        logDebugP("active (Serial=%s)", ParamHMG_dDeviceSerial);
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
    // TODO use configured delay!

    // !_channelActive will result in _running=false, so no need for checking
    if (_running)
    {
        if (delayCheckMillis(_lastRequest_millis, 60 * 1000))
        {
            const bool success = update();
            if (KoHMG_KOdReachable.valueNoSendCompare(success, DPT_Switch))
            {
                KoHMG_KOdReachable.objectWritten();
            }
            _lastRequest_millis = millis();
        }
    }
}

bool HomematicChannel::update()
{
    logDebugP("update()");

    // TODO check moving to module
    String url = "http://";
    url += (const char *)ParamHMG_Host;
    url += ":";
    url += ParamHMG_Port;

    // TODO check moveing to attribute
    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>getParamset</methodName>";
    request += "<params>";

    requestAddParamDeviceSerial(request);
    requestAddParamString(request, "VALUES");

    request += "</params>";
    request += "</methodCall>";

    logDebugP("Device Serial: %s", ParamHMG_dDeviceSerial);
    logDebugP("Read URL POST: %s", url.c_str());

    const uint32_t tStart = millis();

    HTTPClient http;
    /*
#ifdef ARDUINO_ARCH_RP2040
    if (url.startsWith("https://"))
        http.setInsecure();
#endif
    http.begin(url);
    */

    // TODO check using reuse of the connection; needs moving to model
    http.setReuse(false);

    http.begin((const char *)ParamHMG_Host, ParamHMG_Port);

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

    debugLogResponse(http);

    tinyxml2::XMLDocument doc;
    if (doc.Parse(http.getString().c_str()) != tinyxml2::XML_SUCCESS)
    {
        http.end();
        logErrorP("Parsing-Error, ID=%d", doc.ErrorID());
        return false;
    }

    http.end();

    updateKOsFromMethodResponse(doc);
    const uint32_t tEnd = millis();
    const uint32_t tDur_ms = tEnd - tStart;
    logDebugP("[DONE] duration %d ms", tDur_ms);

    return true;
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
    CHECK_NULL(elem, "/methodResponse/params/param/value/struct[]/member")

    return elem;
}

bool HomematicChannel::updateKOsFromMethodResponse(tinyxml2::XMLDocument &doc)
{
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
        if (tinyxml2::XMLElement *doubleElement = memberValue->FirstChildElement("double"))
        {
            const char *pName = memberName->GetText();
            const double value = doubleElement->DoubleText();
            if (strcmp(pName, "ACTUAL_TEMPERATURE") == 0)
            {
                logDebugP("=> ACTUAL_TEMPERATURE=%f", value);
                KoHMG_KOdTempCurrent.valueCompare(value, DPT_Value_Temp);
            }
            else if (strcmp(pName, "BATTERY_STATE") == 0)
            {
                logDebugP("=> BATTERY_STATE=%f", value);
                KoHMG_KOdBatteryVultage.valueCompare(value * 1000, DPT_Value_Volt);
            }
            else if (strcmp(pName, "SET_TEMPERATURE") == 0)
            {
                logDebugP("=> SET_TEMPERATURE=%f", value);
                KoHMG_KOdTempSet.valueCompare(value, DPT_Value_Temp);
            }
            else
            {
                logDebugP("[IGNORE] double-value: %f", value);
            }
        }
        else if (tinyxml2::XMLElement *i4Element = memberValue->FirstChildElement("i4"))
        {
            const char *pName = memberName->GetText();
            const int32_t value = i4Element->IntText();
            if (strcmp(pName, "BOOST_STATE") == 0)
            {
                logDebugP("=> BOOST_STATE=%d", value);
                KoHMG_KOdBoostState.valueCompare(value, DPT_State);
            }
            else if (strcmp(pName, "FAULT_REPORTING") == 0)
            {
                logDebugP("=> FAULT_REPORTING=%d", value);
                KoHMG_KOdError.valueCompare(value, DPT_Alarm);
            }
            else if (strcmp(pName, "VALVE_STATE") == 0)
            {
                logDebugP("=> VALVE_STATE=%d", value);
                KoHMG_KOdValveState.valueCompare(value, DPT_Scaling);
            }
            else
            {
                logDebugP("[IGNORE] i4-value: %d", value);
            }
        }
        else if (tinyxml2::XMLElement *elem = memberValue->FirstChildElement())
        {
            logDebugP("[IGNORE] other value: %s", elem->GetText());
        }
    }

    return true;
}

// TODO check moving to module
bool HomematicChannel::processRssiInfoResponse(tinyxml2::XMLDocument &doc)
{
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

                if (rssi1 != 65536)
                {
                    // TODO calc signal strenght
                }

                if (rssi2 != 65536)
                {
                    // TODO calc signal strenght
                }

            }

        }

    }

    return true;
}

void HomematicChannel::processInputKo(GroupObject &ko)
{
    if (!_channelActive)
        return; // ignore inactive channel

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
                // TODO prepare update within the next seconds!
            }
            break;
        }        
        case HMG_KoKOdTriggerRequest:
        {
            if (KoHMG_KOdTriggerRequest.value(DPT_Trigger))
            {
                const bool success = update();
                KoHMG_KOdReachable.value(success, DPT_Switch);
                _lastRequest_millis = millis();
            }
            break;
        }  
    }
}

bool HomematicChannel::processCommandOverview()
{
    return false;
}

void HomematicChannel::sendSetTemperature(double targetTemperature)
{
    logDebugP("sendSetTemperature(%.3g)", targetTemperature);

    // TODO check moveing to attribute
    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>setValue</methodName>";
    request += "<params>";

    requestAddParamDeviceSerial(request);
    requestAddParamString(request, "SET_TEMPERATURE");
    requestAddParamDouble(request, targetTemperature);

    request += "</params>";
    request += "</methodCall>";

    logDebugP("Set Device %s Temperature to %.3g", ParamHMG_dDeviceSerial, targetTemperature);

    sendRequest(request);
}

void HomematicChannel::sendBoost(bool boost)
{
    logDebugP("sendBoost(%s)", boost ? "true" : "false");

    // TODO check moving to attribute
    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>setValue</methodName>";
    request += "<params>";

    requestAddParamDeviceSerial(request);
    requestAddParamString(request, "BOOST_MODE");
    requestAddParamBoolean(request, boost);

    request += "</params>";
    request += "</methodCall>";

    logDebugP("Set Device %s Boost to %s", ParamHMG_dDeviceSerial, boost ? "true" : "false");

    sendRequest(request);
}

void HomematicChannel::requestAddParamString(arduino::String &request, const char *str)
{
    request += "<param><value><string>";
    request += str;
    request += "</string></value></param>";
}

void HomematicChannel::requestAddParamDeviceSerial(arduino::String &request)
{
    request += "<param><value><string>";
    request += (const char *)ParamHMG_dDeviceSerial; //"OEQ1234567";
    request += ":4";
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

bool HomematicChannel::sendRequest(arduino::String &request)
{
    const uint32_t tStart = millis();

    HTTPClient http;
    http.begin((const char *)ParamHMG_Host, ParamHMG_Port);
    // TODO check using reuse of the connection; needs moving to model
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

    debugLogResponse(http);

    tinyxml2::XMLDocument doc;
    if (doc.Parse(http.getString().c_str()) != tinyxml2::XML_SUCCESS)
    {
        http.end();
        logErrorP("Parsing-Error, ID=%d", doc.ErrorID());
        return false;
    }

    http.end();

    if (!checkSendRequestResponse(doc))
    {
        return false;
    }

    const uint32_t tEnd = millis();
    const uint32_t tDur_ms = tEnd - tStart;
    logDebugP("[DONE] duration %d ms", tDur_ms);
    return true;
}

bool HomematicChannel::checkSendRequestResponse(tinyxml2::XMLDocument &doc)
{
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
    if (root == nullptr)
    {
        logErrorP("Root-Element <methodResponse> is missing!");
        // TODO check error-handling improvement
        return false;
    }

    if (tinyxml2::XMLElement *fault = root->FirstChildElement("fault"))
    {
        // FAIL path in xml: //methodResponse/fault/value/struct/member[]/{name,value/$type}
        // TODO extract error-code!
        logErrorP("Failed! Element /methodResponse/fault/ is present!");
        // TODO check error-handling improvement
        return false;
    }

    tinyxml2::XMLElement *params = root->FirstChildElement("params");
    if (params == nullptr)
    {
        logErrorP("Unexpected Structure. Element /methodResponse/{fault,params} is missing!");
        // TODO check error-handling improvement
        return false;
    }

    tinyxml2::XMLElement *param = params->FirstChildElement("param");
    if (param == nullptr)
    {
        logErrorP("Unexpected Structure. Element /methodResponse/params/param is missing!");
        // TODO check error-handling improvement
        return false;
    }

    tinyxml2::XMLElement *value = param->FirstChildElement("value");
    if (value == nullptr)
    {
        logErrorP("Unexpected Structure. Element /methodResponse/params/param/value is missing!");
        // TODO check error-handling improvement
        return false;
    }
    return true;
}

void HomematicChannel::debugLogResponse(HTTPClient &http)
{
#ifdef OPENKNX_DEBUG
    String response = http.getString();
    logDebugP("response length: %d", response.length());
    const size_t len = response.length();
    const size_t lineLen = 100;
    for (size_t i = 0; i < len; i += lineLen)
    {
        logDebugP("response: %s", response.substring(i, std::min(i + lineLen, len)).c_str());
    }
#endif
}
