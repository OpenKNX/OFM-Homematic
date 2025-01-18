// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#include "HomematicModule.h"

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

void HomematicModule::setup()
{
    logDebugP("setup");
    logIndentUp();
    for (uint8_t i = 0; i < HMG_ChannelCount; i++)
    {
        _channels[i] = new HomematicChannel(i);
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