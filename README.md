# Homematic-Gateway zur Integration in OpenKNX


Von Cornelius Köpp 2024



# Beschreibung
Das Modul ermöglicht einen rudimentären lesenden und schreibenden Zugriff auf ausgewählte Homematic-Geräte (Funkschnittstelle), 
über eine Zentrale mit XML-RPC-Schnittstelle wie z.B. der Homematic CCU2.


> ## Achtung ALPHA-STATUS!
> 
> Der vorliegende Stand dient zur technischen Evaluation:
>
> * Test der XML-RPC Kommunikation auf OpenKNX-Hardware mit Ethernet-Schnittstelle
> * HTTP-Requests gemeinsam mit anderen Modulen
> * Abruf relevanter Werte
> * Auslösen relevanter Steuerungsaktionen
> * Konfiguration
>
> **Ein produktiver Einsatz wird zum aktuellen Zeitpunkt *nicht* empfohlen**, 
> u.A. können die lange blockierenden HTTP-Requests **die Funktion andere Module stören** 
> und es wird bislang ausschließlich eine Kommunikation *ohne* Authentifizierung an der CCU unterstützt.
> Inkompatible Änderungen können ohne Vorankündigung erfolgen.


# Test-Status

| Hardware                                                                      | Status     | Anmerkung |
|-------------------------------------------------------------------------------|------------|-----------|
| RP2040 / [OpenKNX Reg1-ETH](https://github.com/OpenKNX/OpenKNX/wiki/REG1-Eth) | Alpha      | Zusammen mit anderen Modulen als OAM-InternetServices          |
| ESP32                                                                         | ungetestet |           |


# Unterstützte Geräte

Bislang werden ausschließlich Thermostatventile unterstützt. 
Die Anbindung anderer Gerätetypen ist bis auf Weiteres nicht geplant.
Bei Interesse können *nach vorheriger Absprache* Pull-Request zur Funktionsweiterung erstellt werden.

## HM-CC-RT-DN Funk Heizkörperthermostat

### Gelesene Werte

| Wert               | Beschreibung                    |
|--------------------|---------------------------------|
| ACTUAL_TEMPERATURE | Gemessene Temperatur            |
| BATTERY_STAT       | Spannung der Batterien          |
| SET_TEMPERATURE    | Soll-Temperatur                 |
| BOOST_STATE        | Boost-Modus aktiv?              |
| FAULT_REPORTING    | Fehler?                         |
| VALVE_STATE        | Ventil-Position                 |

### Aktionen

* Setzen der SOLL-Temperatur
* Sofortigen Werteabruf von CCU auslösen (sonst in regelmäßigem Intervall)
* Boost-Modus auslösen 




