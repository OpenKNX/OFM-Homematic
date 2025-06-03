# (geplant) v0.3


# 2025-01 Alpha2

* Config/UI Rework and Extension
  * Channel-Activation by Type-Selection (Allows other Device Types later)
  * Separate Checkbox for Temporary Disable
  * Config Params for Update Request Intervall (regular + after write command)
  * Feature #9: Config Param to Switch of Write-Access
* Fixes:
  * Command "hmg runtime"
  * Module in Current OpenKNX-Environment


# 2024-10 Alpha

* Fixes:
  * Firmware
    * Reading Battery Voltage: Fix Name `BATTERY_STATE`
    * Setting Boost: Use Param `BOOST_MODE` and Type `boolean`
    * Prevent Sending Temperature on Startup on Setting Temperature
  * ETS-App-Part
    * Max CCU Port-Number Must be in 16Bit Range
* Features:
  * Control by KO
    * Add: Sending Temperature 
    * Add: Boost-Trigger
    * Add: Check Return of Sending, Send Reachable, for OK Return
    * Add: Trigger to Force Update-Request by KO
    * Prepare Write Control
  * Current Values Read
    * Improve: Send KOs on Change only
    * Improve: Start with 2s Intervals between Channel Updates
    * Improve: Force Quick Update after Boost-Trigger
    * Add: Send Average Rssi-Reading
    * Add: Send Current Set-Temperature to KO
* Refactor
  * Extract Sending HTTP-Requests
  * Parse Response to XML
  * Log Runtime
* Update/Extend Readme with Current State


# 2024-08-20 Prototype 

* First Draft for Usage (as Part of OAM-InternetServices)