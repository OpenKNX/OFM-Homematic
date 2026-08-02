// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2026 Cornelius Koepp

#include "HomematicModule.h"
#include "RpcUtil.h"

HomematicModule::HomematicModule()
{
}

const std::string HomematicModule::name()
{
    return "Homematic";
}

const std::string HomematicModule::version()
{
    return MODULE_Homematic_Version;
}

HomematicChannel* HomematicModule::createChannel(uint8_t _channelIndex)
{
    switch (ParamHMG_dDeviceType)
    {
        case HMG_DEVTYPE__HM_CC_RT_DN__THERMOSTAT: // =1
            return new HomematicChannelThermostat(_channelIndex);
            
        case HMG_DEVTYPE__HM_LC_Sw1_Pl_DN_R1__SWITCH_ACTUATOR: // =6
            return new HomematicChannelSwitchActuator(_channelIndex);
            
        case HMG_DEVTYPE__USER_DEFINED: // =7
            return new HomematicChannelUserDefined(_channelIndex);
            
        case 0: // Disabled/inactive channel
            return new HomematicChannelInactive(_channelIndex);
            
        default:
            logErrorP("Unsupported device type: %d for channel %d - creating inactive channel", ParamHMG_dDeviceType, _channelIndex);
            return new HomematicChannelInactive(_channelIndex);
    }
}

void HomematicModule::setup()
{
    logDebugP("setup");
    logIndentUp();
    for (uint8_t i = 0; i < HMG_ChannelCount; i++)
    {
        _channels[i] = createChannel(i);
    }
    for (uint8_t _channelIndex = 0; _channelIndex < HMG_ChannelCount; _channelIndex++)
    {
        // TODO ensure channel is active (but inactive channels should not create errors or warnings) and include in Channel Implementation
        if ((ParamHMG_dDeviceType != 0) && !ParamHMG_dDisable)
        {
            // init group-assignments by config params
            _devicesUnknown |= ((uint64_t)1 << _channelIndex);
            _groups[0] |= ((uint64_t)1 << _channelIndex);
            _groups[1] |= ((uint64_t)ParamHMG_dGroup1 << _channelIndex);
            _groups[2] |= ((uint64_t)ParamHMG_dGroup2 << _channelIndex);
            _groups[3] |= ((uint64_t)ParamHMG_dGroup3 << _channelIndex);
            _groups[4] |= ((uint64_t)ParamHMG_dGroup4 << _channelIndex);
            _groups[5] |= ((uint64_t)ParamHMG_dGroup5 << _channelIndex);
        }
    }
    for (uint8_t i = 0; i < HMG_ChannelCount; i++)
    {
        _channels[i]->setup();
    }
#ifdef OPENKNX_WEBSERVER
    setupEventRoute();
#endif
    logIndentDown();
}

void HomematicModule::updateDeviceStates(const uint8_t i, const bool unreach, const bool batteryWarn, const bool error)
{
    logDebugP("updateDeviceStates(%d, unreach=%d, batteryWarn=%d, error=%d)", i, unreach, batteryWarn, error);

    const uint64_t maskClear = ~(1ULL << i);

    logTraceP("  _devicesUnknown : %016llX", _devicesUnknown);
    logTraceP("  _devicesUnreach : %016llX", _devicesUnreach);
    logTraceP("  _devicesBattWarn: %016llX", _devicesBatteryWarning);
    logTraceP("  _devicesError   : %016llX", _devicesError);

    _devicesUnreach = (_devicesUnreach & maskClear) | ((uint64_t)unreach << i);
    _devicesBatteryWarning = (_devicesBatteryWarning & maskClear) | ((uint64_t)batteryWarn << i);
    _devicesError = (_devicesError & maskClear) | ((uint64_t)error << i);

    // this device has a known state
    _devicesUnknown &= maskClear;
    
    logTraceP("  bit             : %016llX", ~maskClear);
    logTraceP("  _devicesUnknown : %016llX", _devicesUnknown);
    logTraceP("  _devicesUnreach : %016llX", _devicesUnreach);
    logTraceP("  _devicesBattWarn: %016llX", _devicesBatteryWarning);
    logTraceP("  _devicesError   : %016llX", _devicesError);

    // Update group states for all groups that have all devices known
    for (uint8_t groupIdx = 0; groupIdx < 6; groupIdx++)
    {
        if (_groups[groupIdx])
        {
            bool allDevicesKnown = (_devicesUnknown & _groups[groupIdx]) == 0;
            const bool groupUnreach = (_devicesUnreach & _groups[groupIdx]) != 0;
            const bool groupBatteryWarn = (_devicesBatteryWarning & _groups[groupIdx]) != 0;
            const bool groupError = (_devicesError & _groups[groupIdx]) != 0;
            logDebugP("> Group %d (allKnown=%d): Unreach=%d, BatteryWarn=%d, Error=%d", 
                groupIdx, allDevicesKnown, groupUnreach, groupBatteryWarn, groupError
            );

            // send all negative results as soon as present. positive results when all results are available
            // TODO: Ensure KO-calculation!
            if (allDevicesKnown || groupUnreach)
            {
                knx.getGroupObject(3 * groupIdx + HMG_KoKOGroup0Unreachable).valueCompare(groupUnreach, DPT_Alarm);
            }
            if (allDevicesKnown || groupBatteryWarn)
            {
                knx.getGroupObject(3 * groupIdx + HMG_KoKOGroup0BatteryWarn).valueCompare(groupBatteryWarn, DPT_Alarm);
            }
            if (allDevicesKnown || groupError)
            {
                knx.getGroupObject(3 * groupIdx + HMG_KoKOGroup0Error).valueCompare(groupError, DPT_Alarm);
            }
        }
    }
    
}

void HomematicModule::processAfterStartupDelay()
{
    logDebugP("processAfterStartupDelay");
    // updateRssi();
    logIndentUp();
    for (uint8_t i = 0; i < HMG_ChannelCount; i++)
    {
        _channels[i]->processAfterStartupDelay();
    }
    logIndentDown();
}

void HomematicModule::loop()
{
#ifdef OPENKNX_WEBSERVER
    loopEventReceiver();
#endif
    // TODO optimize
    for (uint8_t i = 0; i < HMG_ChannelCount; i++)
    {
        RUNTIME_MEASURE_BEGIN(_channelLoopRuntimes[i]);
        _channels[i]->loop();
        RUNTIME_MEASURE_END(_channelLoopRuntimes[i]);
    }
}

void HomematicModule::processInputKo(GroupObject &ko)
{
    for (uint8_t i = 0; i < HMG_ChannelCount; i++)
    {
        RUNTIME_MEASURE_BEGIN(_channelInputRuntimes[i]);
        _channels[i]->processInputKo(ko);
        RUNTIME_MEASURE_END(_channelInputRuntimes[i]);
    }
}

bool HomematicModule::updateRssi()
{
    logDebugP("updateRssi()");

    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>rssiInfo</methodName>";
    request += "</methodCall>";

    tinyxml2::XMLDocument doc;
    return hmgClient.sendRequestGetResponseDoc(request, doc) && processRssiInfoResponse(doc);
}

bool HomematicModule::processRssiInfoResponse(tinyxml2::XMLDocument &doc)
{
    const uint32_t tStart = millis();

    tinyxml2::XMLElement *member = hmgClient.getMethodResponseMember(doc);
    if (member == nullptr)
    {
        return false;
    }

    uint32_t count = 0;
    logDebugP("RF-Devices:");
    logIndentUp();
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
        logDebugP("Serial: %s", serial1);
        
        // Copy serial to _scannedDevices if there's space
        if (count < MAX_SCANNED_DEVICES) {
            strncpy(_scannedDevices[count].serial, serial1, 10);
            _scannedDevices[count].serial[10] = '\0';
        }
        // getDeviceDescription(serial1);
        count++;

        /* ignore details, as list of serial is the only relevant information

        if (false) //* strncmp(serial1, (const char *)ParamHMG_dDeviceSerial, ParamHMG_dDeviceSerialLength) == 0
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
            if (true) // //* strncmp(serial2, (const char *)ParamHMG_dDeviceSerial, ParamHMG_dDeviceSerialLength) == 0
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
        */

    }
    logIndentDown();
    _scannedDeviceCount = count;

    logDebugP("[DONE] processRssiInfoResponse() %d ms => found %d", millis() - tStart, count);
    return true;
}

bool HomematicModule::getDeviceDescription(const uint8_t scannedIndex)
{
    const char* serial = _scannedDevices[scannedIndex].serial;
    logDebugP("getDeviceDescription(%d) -> %s", scannedIndex, serial);

    String request = ""; // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    request += "<methodCall>";
    request += "<methodName>getDeviceDescription</methodName>";
    request += "<params>";
    hmgClient.requestAddParamAddress(request, serial, ADDRESS_CHANNEL_NONE);
    request += "</params>";
    request += "</methodCall>";

    tinyxml2::XMLDocument doc;
    return hmgClient.sendRequestGetResponseDoc(request, doc) && process_getDeviceDescription(doc, scannedIndex);
}

bool HomematicModule::process_getDeviceDescription(tinyxml2::XMLDocument &doc, const uint8_t scannedIndex)
{
    const uint32_t channel = 0xff;

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
        /*
        if (tinyxml2::XMLElement *doubleElement = memberValue->FirstChildElement("double"))
        {
            const double value = doubleElement->DoubleText();
            const bool processed = false;
            // const bool processed = _processResponseParamDouble(channel, pName, value);
            logDebugP("%s @%d %24s(d)=%f", (processed ? "=>" : "//"), channel, pName, value);
        }
        else if (tinyxml2::XMLElement *i4Element = memberValue->FirstChildElement("i4"))
        {
            const int32_t value = i4Element->IntText();
            const bool processed = false;
            // const bool processed = _processResponseParamInt32(channel, pName, value);
            logDebugP("%s @%d %24s(i)=%d", (processed ? "=>" : "//"), channel, pName, value);
        }
        else if (tinyxml2::XMLElement *boolElement = memberValue->FirstChildElement("boolean"))
        {
            const bool value = boolElement->IntText();
            const bool processed = false;
            // const bool processed = _processResponseParamBool(channel, pName, value);
            logDebugP("%s @%d %24s(b)=%d", (processed ? "=>" : "//"), channel, pName, value);
        }
        else 
        */
        if (memberValue->FirstChildElement() == nullptr && memberValue->GetText() != nullptr)
        {
            const char* value = memberValue->GetText();
            const bool processed = false;

            // const bool processed = _processResponseParamString(channel, pName, value);
            if (strcmp(pName, "TYPE") == 0)
            {
                // Store device type
                const size_t len = sizeof(_scannedDevices[scannedIndex].type) - 1;
                strncpy(_scannedDevices[scannedIndex].type, value, len);
                _scannedDevices[scannedIndex].type[len] = '\0';
            }
            else if (strcmp(pName, "FIRMWARE") == 0)
            {
                // Store firmware version
                const size_t len = sizeof(_scannedDevices[scannedIndex].firmware) - 1;
                strncpy(_scannedDevices[scannedIndex].firmware, value, len);
                _scannedDevices[scannedIndex].firmware[len] = '\0';
            }
            logDebugP("%s @%d %23s(s)=%s", (processed ? "=>" : "//"), channel, pName, value);
            // TODO collect here: "$TYPE ($FIRMWARE)""
        }
        else
        {
            logDebugP("// @%d %24s(other ignored)", channel, pName);
        }
    }

    logDebugP("[DONE] updateKOsFromMethodResponse() %d ms", millis() - tStart);
    return true;
}

bool HomematicModule::processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
{
    logDebugP("processFunctionProperty(..)");
    if (!knx.configured() || objectIndex != FUNCPROP_OBJECT_INDEX || propertyId != FUNCPROP_ID || length < 1 || data == nullptr || resultData == nullptr)
        return false;

    logDebugP("processFunctionProperty(..) ... // data:");
    logHexDebugP(data, length);

    switch (static_cast<FuncPropCall>(data[0]))
    {
        case FuncPropCall::ScanResult: return processFunctionProperty_ScanResult(resultData, resultLength);
        case FuncPropCall::DeviceInfo: return processFunctionProperty_DevInfo(length, data, resultData, resultLength);
        case FuncPropCall::LastError: return processFunctionProperty_LastError(resultData, resultLength);
    }
    return false; // No valid function property handled
}

bool HomematicModule::processFunctionProperty_ScanResult(uint8_t *resultData, uint8_t &resultLength)
{
    logDebugP("FuncProp[0]: SCAN_RESULT");
    const bool updateResult = updateRssi(); // Ensure list of known devices
    if (!updateResult)
    {
        
    }

    const uint8_t resultCode = FUNCPROP_RESULT_OK;
    const uint16_t found = constrain(_scannedDeviceCount, 0, 0xffff);
    const uint8_t ignored = constrain(_invalidSerialCount, 0, 0xff);

    resultLength = 0;
    resultData[resultLength++] = resultCode;
    resultData[resultLength++] = (found >> 8) & 0xFF;
    resultData[resultLength++] = found & 0xFF;
    resultData[resultLength++] = ignored;

    return true;
}

bool HomematicModule::processFunctionProperty_DevInfo(uint8_t length, uint8_t *data, uint8_t *resultData, uint8_t &resultLength)
{
    _lastFuncProp = 1;
    _lastErrorCode = 0;
    if (length < 2)
    {
        logErrorP("FuncProp[1]: DEV_INFO(missing)");
        _lastErrorCode = 1;
        strncpy(_lastError, "PRM_MIS", sizeof(_lastError) - 1);
        return false;
    }

    const uint8_t devIndex = data[1];
    logDebugP("FuncProp[1]: DEV_INFO(%d)", devIndex);

    resultLength = 0;
    if (devIndex < MAX_SCANNED_DEVICES)
    {
        static_assert(1 + HMG_MAX_SERIAL_LEN + 1 + HMG_MAX_DESCRIPTION_LEN + 1 <= 255, "Result length exceeds maximum of 255 bytes");
        resultData[resultLength++] = 0; // resultCode := OK;
        // use stored serial
        for (uint8_t j = 0; (j < HMG_MAX_SERIAL_LEN) && (_scannedDevices[devIndex].serial[j] != '\0'); j++)
        {
            resultData[resultLength++] = _scannedDevices[devIndex].serial[j];
        }
        resultData[resultLength++] = '\0';

        getDeviceDescription(devIndex);
        for (uint8_t j = 0; (j < HMG_MAX_DESCRIPTION_LEN) && (_scannedDevices[devIndex].type[j] != '\0') ; j++)
        {
            resultData[resultLength++] = _scannedDevices[devIndex].type[j];
        }
        resultData[resultLength++] = '\0';
        return true;
    }
    else
    {
        resultData[resultLength++] = FUNCPROP_RESULT_FAIL; // resultCode := FAIL ">= MAX_SCANNED_DEVICES" // TODO define error-code-list/system and constants
        // Note: following content would not be used in ETS on result!=0
        _lastErrorCode = 2;
        strncpy(_lastError, "IDX>MAX", sizeof(_lastError) - 1);
        return false;
    }
}

bool HomematicModule::processFunctionProperty_LastError(uint8_t *resultData, uint8_t &resultLength)
{
    logDebugP("FuncProp[0]: LAST_ERR");

    const uint8_t resultCode = FUNCPROP_RESULT_OK;

    resultLength = 0;
    resultData[resultLength++] = resultCode;
    resultData[resultLength++] = _lastFuncProp;
    resultData[resultLength++] = _lastErrorCode;
    for (uint8_t j = 0; j < sizeof(_lastError) && _lastError[j] != '\0'; j++)
    {
        resultData[resultLength++] = _lastError[j];
    }
    resultData[resultLength++] = '\0';

    return true;
}

void HomematicModule::showHelp()
{
    if (knx.configured())
    {
        // TODO Check and refine command definitions after first tests and extension!
        openknx.console.printHelpLine("hmg ccuscan",    "List devices known by CCU");
        openknx.console.printHelpLine("hmgNN",          "Device overview");
        openknx.console.printHelpLine("hmgNN update",   "Update device state");
        openknx.console.printHelpLine("hmgNN temp=CC",  "Set target temperature");
    }
#ifdef OPENKNX_RUNTIME_STAT
    openknx.console.printHelpLine("hmg runtime",    "Show detailed runtime statistic");
#endif
}

bool HomematicModule::processCommand(const std::string cmd, bool diagnoseKo)
{
    // configured is not required for runtime-command
#ifdef OPENKNX_RUNTIME_STAT
    if (cmd == "hmg runtime")
    {
        logInfoP("HMG Runtime Statistics: (Uptime=%dms)", millis());
        logIndentUp();
        OpenKNX::Stat::RuntimeStat::showStatHeader();
        char labelLoop[8 + 1] = "Ch00Loop";
        char labelInput[8 + 1] = "Ch00Inpt";
        for (uint8_t i = 0; i < HMG_ChannelCount; i++)
        {
            labelLoop[2] = labelInput[2] = '0' + i / 10;
            labelLoop[3] = labelInput[3] = '0' + i % 10;
            _channelLoopRuntimes[i].showStat(labelLoop, 0, true, true);
            _channelInputRuntimes[i].showStat(labelInput, 0, true, true);
        }
        logIndentDown();
        return true;
    }
#endif

    if (!knx.configured())
        return false;

    const size_t cmdLength = cmd.length();
    if (cmdLength >= 5 && cmd.substr(0, 3) == "hmg")
    {
        if (cmd == "hmg ccuscan")
        {
            updateRssi();
            return true;
        }

        if (!std::isdigit(cmd[3]) || !std::isdigit(cmd[4]))
        {
            logErrorP("=> invalid channel '%s'!", cmd.substr(3, 2).c_str());
            return false;
        }

        const uint16_t channelIdx = std::stoi(cmd.substr(3, 2)) - 1;
        if (channelIdx < HMG_ChannelCount)
        {
            if (cmdLength == 5)
            {
                logDebugP("=> Channel<%u> overview!", (channelIdx + 1));
                return _channels[channelIdx]->processCommandOverview();
            }
        }
        else
        {
            logInfoP("=> unused channel %u!", channelIdx + 1);
        }
    }
    return false;
}

// #ifdef OPENKNX_WEBSERVER

// Helper: parse event address format "SERIAL:CHANNEL" into components
// Returns false if address is malformed
static bool parseEventAddress(const char* address, std::string& outSerial, uint8_t& outChannel)
{
    if (address == nullptr || address[0] == '\0')
        return false;

    const char* colonPos = strchr(address, ':');
    if (colonPos == nullptr || colonPos == address)
        return false;

    outSerial.assign(address, colonPos - address);
    outChannel = (uint8_t)strtoul(colonPos + 1, nullptr, 10);
    return true;
}

// Helper: extract typed value from XML <value> element
// Checks for typed elements (<boolean>, <i4>, <double>, <string>) and returns the type
// If no typed element found, treats bare text as string
// Returns type name ("boolean", "i4", "double", "string") or nullptr if no value found
static const char* extractTypedValue(tinyxml2::XMLElement *value, std::string& outValue)
{
    if (value == nullptr)
        return nullptr;

    // Check for typed elements in priority order
    tinyxml2::XMLElement *typed = value->FirstChildElement("boolean");
    if (typed && typed->GetText())
    {
        outValue = typed->GetText();
        return "boolean";
    }

    typed = value->FirstChildElement("i4");
    if (typed && typed->GetText())
    {
        outValue = typed->GetText();
        return "i4";
    }

    typed = value->FirstChildElement("double");
    if (typed && typed->GetText())
    {
        outValue = typed->GetText();
        return "double";
    }

    typed = value->FirstChildElement("string");
    if (typed && typed->GetText())
    {
        outValue = typed->GetText();
        return "string";
    }

    // Bare text (implicit string)
    const char *text = value->GetText();
    if (text)
    {
        outValue = text;
        return "string";
    }

    return nullptr;
}

void HomematicModule::setupEventRoute()
{
    if (_eventRouteRegistered)
        return;
    _eventRouteRegistered = true;

    _eventInterfaceId = std::string("OFM-Homematic-") + openknx.info.humanSerialNumber();

    openknxNetwork.webserver.addRoute(OpenKNX::Network::WEB_POST, "/HMG/events",
        [this](OpenKNX::Network::WebRequest &req, OpenKNX::Network::WebResponse &res) {
            handleEventRequest(req, res);
        });

    logInfoP("Event route /HMG/events registered (interface_id=%s)", _eventInterfaceId.c_str());
}

void HomematicModule::loopEventReceiver()
{
    if (!_eventRouteRegistered || !openknxNetwork.established() || !openknxNetwork.webserver.isRunning())
        return;

    // Prototype: (re-)register once at start, and again whenever no event arrived within the timeout.
    if (!_eventReceiverRegistered || (millis() - _lastEventOrRegisterMs > HMG_EVENT_RENEW_TIMEOUT_MS))
    {
        registerEventReceiver();
    }
}

void HomematicModule::registerEventReceiver()
{
    String url = "http://";
    url += openknxNetwork.localIP().toString();
    url += "/HMG/events";

    logInfoP("Registering event receiver at CCU: init(%s, %s)", url.c_str(), _eventInterfaceId.c_str());

    _eventReceiverRegistered = hmgClient.rpcInitEventReceiver(url.c_str(), _eventInterfaceId.c_str());
    _lastEventOrRegisterMs = millis();

    if (!_eventReceiverRegistered)
    {
        logErrorP("Registering event receiver at CCU failed");
    }
}

void HomematicModule::handleEventRequest(OpenKNX::Network::WebRequest &req, OpenKNX::Network::WebResponse &res)
{
    static const char *emptyMethodResponse =
        "<?xml version=\"1.0\"?><methodResponse><params><param><value></value></param></params></methodResponse>";

    tinyxml2::XMLDocument doc;
    if (req.body() == nullptr || doc.Parse((const char *)req.body(), req.bodyLength()) != tinyxml2::XML_SUCCESS)
    {
        logErrorP("Failed to parse incoming XML-RPC event request");
        res.setContentType("text/xml");
        res.send(emptyMethodResponse);
        return;
    }

    tinyxml2::XMLElement *methodCall = doc.FirstChildElement("methodCall");
    tinyxml2::XMLElement *methodNameElem = methodCall ? methodCall->FirstChildElement("methodName") : nullptr;
    const char *methodName = methodNameElem ? methodNameElem->GetText() : nullptr;

    uint32_t multicallCount = 0;

    if (methodName && strcmp(methodName, "event") == 0)
    {
        tinyxml2::XMLElement *params = methodCall->FirstChildElement("params");
        tinyxml2::XMLElement *paramValues[4] = {nullptr, nullptr, nullptr, nullptr};
        uint8_t n = 0;
        for (tinyxml2::XMLElement *param = params ? params->FirstChildElement("param") : nullptr; param && n < 4; param = param->NextSiblingElement("param"))
        {
            paramValues[n++] = param->FirstChildElement("value");
        }
        processEventValues(paramValues);
        _lastEventOrRegisterMs = millis();
    }
    else if (methodName && strcmp(methodName, "system.multicall") == 0)
    {
        // path: params/param/value/array/data/value[]/struct/member[]{name,value}
        tinyxml2::XMLElement *params = methodCall->FirstChildElement("params");
        tinyxml2::XMLElement *param = params ? params->FirstChildElement("param") : nullptr;
        tinyxml2::XMLElement *value = param ? param->FirstChildElement("value") : nullptr;
        tinyxml2::XMLElement *array = value ? value->FirstChildElement("array") : nullptr;
        tinyxml2::XMLElement *data = array ? array->FirstChildElement("data") : nullptr;

        for (tinyxml2::XMLElement *entry = data ? data->FirstChildElement("value") : nullptr; entry; entry = entry->NextSiblingElement("value"))
        {
            multicallCount++;
            tinyxml2::XMLElement *entryStruct = entry->FirstChildElement("struct");
            if (entryStruct == nullptr)
                continue;

            const char *entryMethodName = nullptr;
            tinyxml2::XMLElement *entryParamsData = nullptr;
            for (tinyxml2::XMLElement *member = entryStruct->FirstChildElement("member"); member; member = member->NextSiblingElement("member"))
            {
                tinyxml2::XMLElement *nameElem = member->FirstChildElement("name");
                tinyxml2::XMLElement *valueElem = member->FirstChildElement("value");
                if (nameElem == nullptr || valueElem == nullptr)
                    continue;

                const char *memberName = nameElem->GetText();
                if (memberName == nullptr)
                    continue;

                if (strcmp(memberName, "methodName") == 0)
                {
                    entryMethodName = valueElem->GetText();
                    if (entryMethodName == nullptr)
                    {
                        if (tinyxml2::XMLElement *strElem = valueElem->FirstChildElement("string"))
                            entryMethodName = strElem->GetText();
                    }
                }
                else if (strcmp(memberName, "params") == 0)
                {
                    if (tinyxml2::XMLElement *entryArray = valueElem->FirstChildElement("array"))
                        entryParamsData = entryArray->FirstChildElement("data");
                }
            }

            if (entryMethodName && strcmp(entryMethodName, "event") == 0)
            {
                tinyxml2::XMLElement *paramValues[4] = {nullptr, nullptr, nullptr, nullptr};
                uint8_t n = 0;
                for (tinyxml2::XMLElement *entryValue = entryParamsData ? entryParamsData->FirstChildElement("value") : nullptr; entryValue && n < 4; entryValue = entryValue->NextSiblingElement("value"))
                {
                    paramValues[n++] = entryValue;
                }
                processEventValues(paramValues);
            }
        }
        _lastEventOrRegisterMs = millis();
    }
    else
    {
        logDebugP("Ignoring incoming XML-RPC call '%s'", methodName ? methodName : "?");
    }

    res.setContentType("text/xml");
    if (multicallCount == 0)
    {
        res.send(emptyMethodResponse);
    }
    else
    {
        std::string response = "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data>";
        for (uint32_t i = 0; i < multicallCount; i++)
            response += "<value></value>";
        response += "</data></array></value></param></params></methodResponse>";
        res.send(response.c_str());
    }
}

void HomematicModule::processEventValues(tinyxml2::XMLElement *paramValues[4])
{
    // Parameter 0: Token (String)
    const char *token = paramValues[0] ? paramValues[0]->GetText() : nullptr;
    if (!token)
        token = "";

    // Parameter 1: Address "SERIAL:CHANNEL" (String)
    const char *addressStr = paramValues[1] ? paramValues[1]->GetText() : nullptr;
    if (!addressStr)
    {
        logWarningP("Event missing address parameter");
        return;
    }

    std::string serial;
    uint8_t channel;
    if (!parseEventAddress(addressStr, serial, channel))
    {
        logWarningP("Invalid event address format (expected SERIAL:CHANNEL): %s", addressStr);
        return;
    }

    // Parameter 2: Parameter key (String)
    const char *paramKey = paramValues[2] ? paramValues[2]->GetText() : nullptr;
    if (!paramKey || paramKey[0] == '\0')
    {
        logWarningP("Event missing parameter key");
        return;
    }

    // Parameter 3: Value (can be typed: boolean, i4, double, or string)
    std::string paramValueStr;
    const char *valueType = extractTypedValue(paramValues[3], paramValueStr);
    if (valueType == nullptr)
    {
        logWarningP("Event missing parameter value for key %s", paramKey);
        return;
    }

    logDebugP("HMG event: serial=%s ch=%d key=%s type=%s value=%s", serial.c_str(), channel, paramKey, valueType, paramValueStr.c_str());

    // Dispatch based on XML type tag
    if (strcmp(valueType, "boolean") == 0)
    {
        bool bval = strcasecmp(paramValueStr.c_str(), "true") == 0 || paramValueStr == "1";
        _processEventParamBool(serial.c_str(), channel, paramKey, bval);
    }
    else if (strcmp(valueType, "i4") == 0)
    {
        int32_t ival = (int32_t)strtol(paramValueStr.c_str(), nullptr, 10);
        _processEventParamInt32(serial.c_str(), channel, paramKey, ival);
    }
    else if (strcmp(valueType, "double") == 0)
    {
        double dval = strtod(paramValueStr.c_str(), nullptr);
        _processEventParamDouble(serial.c_str(), channel, paramKey, dval);
    }
    else
    {
        logDebugP("Unsupported parameter type '%s': key=%s value=%s", valueType, paramKey, paramValueStr.c_str());
    }
}

// Interne Hilfsfunktion: generische Verarbeitung für alle Datentypen
template<typename ValueType>
bool HomematicModule::_processEventParamGeneric(
    const char* serial, 
    uint8_t channel, 
    const char* pName, 
    ValueType value,
    bool (HomematicChannel::*processFn)(const uint8_t, const char*, ValueType, const bool))
{
    uint8_t countProcessed = 0;
    for (uint8_t _channelIndex = 0; _channelIndex < HMG_ChannelCount; _channelIndex++)
    {
        if (_channels[_channelIndex]->getSerial() == serial)
        {
            // TODO must replace last param, when used for reading!
            if ((_channels[_channelIndex]->*processFn)(channel, pName, value, true))
                countProcessed++;
        }
    }
    return countProcessed > 0;
}

// Spezialisierte Funktionen: nur Logging + Delegation
bool HomematicModule::_processEventParamDouble(const char* serial, uint8_t channel, const char* pName, double value)
{
    logInfoP("HMG event: serial=%s channel=%d key=%s value<double>=%f", serial, channel, pName, value);
    return _processEventParamGeneric(serial, channel, pName, value, &HomematicChannel::_processResponseParamDouble);
}

bool HomematicModule::_processEventParamInt32(const char* serial, uint8_t channel, const char* pName, int32_t value)
{
    logInfoP("HMG event: serial=%s channel=%d key=%s value<int32>=%d", serial, channel, pName, value);
    return _processEventParamGeneric(serial, channel, pName, value, &HomematicChannel::_processResponseParamInt32);
}

bool HomematicModule::_processEventParamBool(const char* serial, uint8_t channel, const char* pName, bool value)
{
    logInfoP("HMG event: serial=%s channel=%d key=%s value<bool>=%s", serial, channel, pName, value ? "true" : "false");
    return _processEventParamGeneric(serial, channel, pName, value, &HomematicChannel::_processResponseParamBool);
}



// #endif // OPENKNX_WEBSERVER

HomematicModule openknxHomematicModule;