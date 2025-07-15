// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#pragma once
#include "HomematicChannel.h"

// KOs for device type HM-LC-Sw1-Pl-DN-R1 "Funk-Zwischenstecker-Schaltaktor 1fach"
#define HMG_KoKOdSwitch           HMG_KoKOd__KO__7
#define HMG_KoKOdSwitchStair      HMG_KoKOd__KO__8
#define HMG_KoKOdLock             HMG_KoKOd__KO__10
#define KoHMG_KOdSwitchState      KoHMG_KOd__KO__6
#define KoHMG_KOdSwitch           KoHMG_KOd__KO__7
#define KoHMG_KOdSwitchStair      KoHMG_KOd__KO__8
#define KoHMG_KOdLockState        KoHMG_KOd__KO__9
#define KoHMG_KOdLock             KoHMG_KOd__KO__10

/**
 * Device Type HM-LC-Sw1-Pl-DN-R1 "Funk-Zwischenstecker-Schaltaktor 1fach"
 * Handles switch control, stair lighting, and lock/inhibit functionality.
 */
class HomematicChannelSwitchActuator : public HomematicChannel
{
  private:
    // Device type specific
    void sendSwitchStairlight(bool switchStair);

  protected:
    // Implementation of pure virtual methods from base class
    void processDeviceSpecificInputKo(GroupObject &ko) override;
    uint8_t getDeviceChannel() const override { return 1; }
    
    // Parameter-handler
    bool processResponseParamBool(uint8_t channel, const char* pName, bool value) override;

  public:
    explicit HomematicChannelSwitchActuator(uint8_t index);
    const std::string name() override;
};
