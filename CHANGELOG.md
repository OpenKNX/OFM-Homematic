# (geplant) v0.3

* Feature
  * (Draf) Support for additional device type: HM-LC-Sw1-Pl-DN-R1
    * Reading and writing `STATE`
    * Trigger Stairlight with writing `ON_TIME` before `STATE`
    * Reading and writing `INHIBIT`
  * (Draft) Custom device type with user defined params
* ETS-UI Improvements
  * New icon for channels (replace thermostat with generic radio device)
  * Special layout for device selection
* Add Application Description 
* Refactoring
  * Generic handling of read and write commands to device 
  * Split implementation into base and device-type child classes
  * Allow requesting multiple device-channels

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