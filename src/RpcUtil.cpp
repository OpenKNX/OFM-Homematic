// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#include "RpcUtil.h"

/**
 * @brief Constructor
 */
RpcUtil::RpcUtil()
{
}

/**
 * @brief Returns the name for logging
 */
const std::string RpcUtil::name()
{
    return "HMG<Client>";
}

tinyxml2::XMLElement* RpcUtil::getMethodResponseMember(tinyxml2::XMLDocument &doc)
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

bool RpcUtil::rpcSetValueDouble(const char* deviceSerial, const uint8_t channel, const char * paramName, double value)
{
    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>setValue</methodName>";
    request += "<params>";
    requestAddParamAddress(request, deviceSerial, channel);
    requestAddParamString(request, paramName);
    requestAddParamDouble(request, value);
    request += "</params>";
    request += "</methodCall>";

    logDebugP("XML-RPC call: setValue(%s, %s, %.3f)", deviceSerial, paramName, value);

    return sendRequestCheckResponseOk(request);
}

bool RpcUtil::rpcSetValueBool(const char* deviceSerial, const uint8_t channel, const char * paramName, bool value)
{
    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>setValue</methodName>";
    request += "<params>";
    requestAddParamAddress(request, deviceSerial, channel);
    requestAddParamString(request, paramName);
    requestAddParamBoolean(request, value);
    request += "</params>";
    request += "</methodCall>";

    logDebugP("XML-RPC call: setValue(%s, %s, %s)", deviceSerial, paramName, value ? "true" : "false");

    return sendRequestCheckResponseOk(request);
}

bool RpcUtil::rpcSetValueInteger4(const char* deviceSerial, const uint8_t channel, const char * paramName, int32_t value)
{
    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>setValue</methodName>";
    request += "<params>";
    requestAddParamAddress(request, deviceSerial, channel);
    requestAddParamString(request, paramName);
    requestAddParamInteger4(request, value);
    request += "</params>";
    request += "</methodCall>";

    logDebugP("XML-RPC call: setValue(%s, %s, %d)", deviceSerial, paramName, value);

    return sendRequestCheckResponseOk(request);
}

void RpcUtil::requestAddParamString(arduino::String &request, const char *str)
{
    request += "<param><value><string>";
    request += str;
    request += "</string></value></param>";
}

void RpcUtil::requestAddParamAddress(arduino::String &request, const char* deviceSerial, uint8_t channel)
{
    request += "<param><value><string>";
    request += deviceSerial;
    if (channel != ADDRESS_CHANNEL_NONE)
    {
        request += ":";
        request += String(channel);
    }
    request += "</string></value></param>";
}

void RpcUtil::requestAddParamDouble(arduino::String &request, double value)
{
    request += "<param><value><double>";
    request += value;
    request += "</double></value></param>";
}

void RpcUtil::requestAddParamInteger4(arduino::String &request, int32_t value)
{
    request += "<param><value><i4>";
    request += value;
    request += "</i4></value></param>";
}

void RpcUtil::requestAddParamBoolean(arduino::String &request, boolean value)
{
    request += "<param><value><boolean>";
    request += value ? 1 : 0;
    request += "</boolean></value></param>";
}

/**
 * @brief Sends an XML-RPC request to the Homematic device and parses the XML response.
 * 
 * @param request - The XML-RPC request as a string.
 * @param doc - Reference to a tinyxml2::XMLDocument where the response will be parsed.
 * @return true - if the request was successful and the response was parsed without errors, false otherwise.
 */
bool RpcUtil::sendRequestGetResponseDoc(arduino::String &request, tinyxml2::XMLDocument &doc)
{
    const uint32_t tStart = millis();

    // logDebugP("Read URL POST: http://%s:%d", (const char *)ParamHMG_Host, ParamHMG_Port);

    HTTPClient http;
    /*
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
    // FIXME: will consume http-response string debugLogResponse(http);

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

bool RpcUtil::sendRequestCheckResponseOk(arduino::String &request)
{
    tinyxml2::XMLDocument doc;
    return sendRequestGetResponseDoc(request, doc) && checkSendRequestResponse(doc);
}

bool RpcUtil::checkSendRequestResponse(tinyxml2::XMLDocument &doc)
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

void RpcUtil::debugLogResponse(HTTPClient &http, bool logResponse)
{
#ifdef OPENKNX_DEBUG
    if (logResponse)
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

RpcUtil hmgClient;
