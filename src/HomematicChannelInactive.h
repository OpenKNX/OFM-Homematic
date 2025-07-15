// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2024-2025 Cornelius Koepp

#pragma once
#include "HomematicChannel.h"

/**
 * HomematicChannel implementation for inactive/disabled device
 * This class provides a NO-OP implementation for all device-specific methods
 * and allows a clean module implementation without null checks.
 */
class HomematicChannelInactive : public HomematicChannel
{
  protected:
    // Implementation of pure virtual methods from base class (all no-ops)
    void processDeviceSpecificInputKo(GroupObject &ko) override;
    uint8_t getDeviceChannel() const override { return 0; }

  public:
    explicit HomematicChannelInactive(uint8_t index);
    const std::string name() override;
    
    // Override base class methods to provide no-op implementations
    void setup() override;
    void loop() override;
};
