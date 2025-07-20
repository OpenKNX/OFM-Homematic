// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#pragma once
#include "HomematicChannelThermostat.h"
#include "HomematicChannelSwitchActuator.h"
#include "HomematicChannelUserDefined.h"
#include "HomematicChannelInactive.h"
#include "OpenKNX.h"
// always include for RUNTIME_MEASURE_{BEGIN,END}
#include "OpenKNX/Stat/RuntimeStat.h"

class HomematicModule : public OpenKNX::Module
{
  private:
    HomematicChannel *_channels[HMG_ChannelCount];
#ifdef OPENKNX_RUNTIME_STAT
    OpenKNX::Stat::RuntimeStat _channelLoopRuntimes[HMG_ChannelCount];
    OpenKNX::Stat::RuntimeStat _channelInputRuntimes[HMG_ChannelCount];
#endif

    // aggregatet device state
    static_assert(HMG_ChannelCount <= 64, "HMG_ChannelCount must not exceed 64");
    // uint64_t _groups[6] = {~0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}; 
    uint64_t _groups[6] = {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL}; 
    uint64_t _devicesUnknown = 0;
    uint64_t _devicesUnreach = 0;
    uint64_t _devicesBatteryWarning = 0;
    uint64_t _devicesError = 0;

    // Factory method for creating channels
    HomematicChannel* createChannel(uint8_t _channelIndex);

    bool updateRssi();
    bool processRssiInfoResponse(tinyxml2::XMLDocument &doc);

    bool getDeviceDescription(const char *serial);
    bool process_getDeviceDescription(tinyxml2::XMLDocument &doc);

  public:
    HomematicModule();
    const std::string name() override;
    const std::string version() override;

    // configured only
    void setup() override;
    void processAfterStartupDelay() override;
    void loop() override;

    void processInputKo(GroupObject &ko) override;

    void showHelp() override;
    bool processCommand(const std::string cmd, bool diagnoseKo);

    void updateDeviceStates(const uint8_t i, const bool unreach, const bool batteryWarn, const bool error);
};

extern HomematicModule openknxHomematicModule;