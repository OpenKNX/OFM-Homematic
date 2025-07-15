// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#pragma once
#include "HomematicChannel.h"

// KOs for device type HM-CC-RT-DN (Thermostat)
#define HMG_KoKOdTempSet          HMG_KoKOd__KO__8
#define HMG_KoKOdBoostTrigger     HMG_KoKOd__KO__10
#define KoHMG_KOdTempCurrent      KoHMG_KOd__KO__6
#define KoHMG_KOdTempSetCurrent   KoHMG_KOd__KO__7
#define KoHMG_KOdTempSet          KoHMG_KOd__KO__8
#define KoHMG_KOdBoostState       KoHMG_KOd__KO__9
#define KoHMG_KOdBoostTrigger     KoHMG_KOd__KO__10
#define KoHMG_KOdValveState       KoHMG_KOd__KO__11

/**
 * Device Type HM-CC-RT-DN "Thermostat"
 * Handles temperature control, boost mode, and valve state monitoring.
 */
class HomematicChannelThermostat : public HomematicChannel
{
  private:
    // Device type specific
    void sendSetTemperature(double targetTemperature);
    void sendBoost(bool boost);

  protected:
    // Implementation of pure virtual methods from base class
    void processDeviceSpecificInputKo(GroupObject &ko) override;
    uint8_t getDeviceChannel() const override { return 4; }
    
    // Parameter-handler
    bool processResponseParamDouble(uint8_t channel, const char* pName, double value) override;
    bool processResponseParamInt32(uint8_t channel, const char* pName, int32_t value) override;

  public:
    explicit HomematicChannelThermostat(uint8_t index);
    const std::string name() override;
};
