// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

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
        _channels[i]->setup();
    }
    logIndentDown();
}

void HomematicModule::processAfterStartupDelay()
{
    logDebugP("processAfterStartupDelay");
    logIndentUp();
    for (uint8_t i = 0; i < HMG_ChannelCount; i++)
    {
        _channels[i]->processAfterStartupDelay();
    }
    logIndentDown();
}

void HomematicModule::loop()
{
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
        if (false /*strcmp(serial1, (const char *)ParamHMG_dDeviceSerial) == 0*/)
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
                /*
                // TODO REMOVE // check moving to module OR replace with getParametSet for Channel :0
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
                */
            }

        }

    }

    logDebugP("[DONE] processRssiInfoResponse() %d ms", millis() - tStart);
    return true;
}

void HomematicModule::showHelp()
{
    // TODO Check and refine command definitions after first tests and extension!
    openknx.console.printHelpLine("hmgNN",          "Device overview");
    openknx.console.printHelpLine("hmgNN update",   "Update device state");
    openknx.console.printHelpLine("hmgNN temp=CC",  "Set target temperature");
}

bool HomematicModule::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd.substr(0, 3) == "hmg")
    {
        if (cmd.length() >= 5)
        {
            if (!std::isdigit(cmd[3]) || !std::isdigit(cmd[4]))
            {
                logErrorP("=> invalid channel-number '%s'!", cmd.substr(3, 2).c_str());
                return false;
            }

            const uint16_t channelIdx = std::stoi(cmd.substr(3, 2)) - 1;
            if (channelIdx < HMG_ChannelCount)
            {
                if (cmd.length() == 5)
                {
                    logDebugP("=> Channel<%u> overview!", (channelIdx + 1));
                    return _channels[channelIdx]->processCommandOverview();
                }
            }
            else
            {
                logInfoP("=> unused channel-number %u!", channelIdx + 1);
            }
        }
#ifdef OPENKNX_RUNTIME_STAT
        else if (cmd == "hmg runtime")
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
        }
#endif
    }
    return false;
}

HomematicModule openknxHomematicModule;