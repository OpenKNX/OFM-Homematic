// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2026 Cornelius Koepp

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
    // TODO reserve expected length
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

bool RpcUtil::rpcInitEventReceiver(const char* url, const char* interfaceId)
{
    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>init</methodName>";
    request += "<params>";
    requestAddParamString(request, url);
    requestAddParamString(request, interfaceId);
    request += "</params>";
    request += "</methodCall>";

    logDebugP("XML-RPC call: init(%s, %s)", url, interfaceId);

    return sendRequestCheckResponseOk(request);
}

void RpcUtil::requestAddParamString(String &request, const char *str)
{
    request += "<param><value><string>";
    request += str;
    request += "</string></value></param>";
}

void RpcUtil::requestAddParamAddress(String &request, const char* deviceSerial, uint8_t channel)
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

void RpcUtil::requestAddParamDouble(String &request, double value)
{
    request += "<param><value><double>";
    request += value;
    request += "</double></value></param>";
}

void RpcUtil::requestAddParamInteger4(String &request, int32_t value)
{
    request += "<param><value><i4>";
    request += value;
    request += "</i4></value></param>";
}

void RpcUtil::requestAddParamBoolean(String &request, boolean value)
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
bool RpcUtil::sendRequestGetResponseDoc(String &request, tinyxml2::XMLDocument &doc)
{
    const uint32_t tStart = millis();

    // URL has format "http://{$Host}:{$Port}"
    std::string url = "http://";
    url += (const char *)ParamHMG_Host;
    url += ":";
    url += std::to_string(ParamHMG_Port);

    bool done = false;
    OpenKNX::Network::Webclient::Response result;

    const bool queued = openknxNetwork.webclient.post(url)
                            .contentType("text/xml")
                            .header("Accept", "text/xml")
                            .body(request.c_str(), request.length())
                            .ignoreHeaders()
                            .maxBodySize(HMG_RPC_MAX_RESPONSE_SIZE)
                            .onDone([&done, &result](const OpenKNX::Network::Webclient::Response &res) {
                                result = res;
                                done = true;
                            })
                            .send();
    if (!queued)
    {
        logErrorP("Webclient request could not be queued");
        return false;
    }

    // TODO: still processed synchronously (blocking) for now - pump the webclient loop ourselves until onDone fires.
    //       Switch HomematicModule/HomematicChannel to fully asynchronous processing in a follow-up step.
    while (!done)
    {
        openknxNetwork.webclient.loop();
        // TODO check and replace with full async implementation
        yield(); // let other tasks/background processing (USB, watchdog, ...) run while we wait
        if (delayCheckMillis(tStart, OPENKNX_WEBCLIENT_TIMEOUT))
        {
            logErrorP("Webclient request timed out");
            return false;
        }
    }

    // AFTER done==true

    logDebugP("[DONE] duration request %d ms", millis() - tStart);

    if (!result.success())
    {
        logErrorP("POST returned http %d", result.status());
        return false;
    }
    if (result.bodyIncomplete())
    {
        logErrorP("Response body exceeded %d bytes, truncated", HMG_RPC_MAX_RESPONSE_SIZE);
        return false;
    }

    debugLogResponse(result.body(), false);

    const uint32_t tStart3 = millis();
    if (doc.Parse(result.body().c_str()) != tinyxml2::XML_SUCCESS)
    {
        logErrorP("Parsing-Error, ID=%d", doc.ErrorID());
        // TODO save error
        return false;
    }
    logDebugP("[DONE] parse %d ms", millis() - tStart3);

    return true;
}

bool RpcUtil::sendRequestCheckResponseOk(String &request)
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

void RpcUtil::debugLogResponse(const std::string response, bool logResponse /* = false */)
{
#ifdef OPENKNX_DEBUG
    if (logResponse)
    {
        const uint32_t tStart2 = millis();

        logDebugP("response length: %d", response.length());
        const size_t len = response.length();
        const size_t lineLen = 100;
        for (size_t i = 0; i < len; i += lineLen)
        {
            logDebugP("response: %s", response.substr(i, std::min(i + lineLen, len)).c_str());
        }

        logDebugP("[DONE] duration log response %d ms", millis() - tStart2);
    }
#endif
}

RpcUtil hmgClient;
