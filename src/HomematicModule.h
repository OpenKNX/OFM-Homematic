// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024 Cornelius Koepp

#pragma once
#include "HomematicChannel.h"
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
};

extern HomematicModule openknxHomematicModule;