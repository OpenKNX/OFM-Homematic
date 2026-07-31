# OFM-Homematic: Gateway Modul zur Integration in OpenKNX


Von Cornelius Köpp 2024-2026



# Beschreibung
Das Modul ermöglicht einen rudimentären lesenden und schreibenden Zugriff auf ausgewählte Homematic-Geräte (Funkschnittstelle), 
über eine Zentrale mit XML-RPC-Schnittstelle wie z.B. der Homematic CCU2.

Zur Konfiguration in der ETS siehe [Applikationsbeschreibung](doc/Applikationsbeschreibung-Homematic.md)


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

| Architektur / Hardware                                                        | Status     | Anmerkung                                     |
|-------------------------------------------------------------------------------|------------|-----------------------------------------------|
| RP2040 / [OpenKNX Reg1-ETH](https://github.com/OpenKNX/OpenKNX/wiki/REG1-Eth) | Alpha      | Zusammen mit anderen Modulen in OAM-Homematic |
| ESP32                                                                         | ungetestet |                                               |


## Kommunikationsstruktur

Die Kommunikation mit den Homematic-Geräten ist, mit der aktuellen Implementierung, 
ausschließlich über eine CCU2 oder andere Zentrale mit kompatibler XML-RPC-Schnittstelle möglich:
```
                                                                           ┌─────────────────┐
                                                                           │ Homematic-Gerät │
┌─────────┐    ┌───────────────┐  XML-RPC  ┌─────────────────┐   BidCoS    └─────────────────┘
│ KNX-Bus │<──>│ OFM-Homematic │<─────────>│  Homematic/CCU2 │< ─ ─ ─ ─ >  : ...             : 
└─────────┘    └───────────────┘   LAN/IP  └─────────────────┘   868Mhz    ┌─────────────────┐
                                                                           │ Homematic-Gerät │
                                                                           └─────────────────┘
                                           
               \____OpenKnx____/           \____________________Homematic____________________/
```


# Unterstützte Geräte

Bislang werden 
Funk-Thermostatventile des Types HM-CC-RT-DN, 
Schaltbare Zwischensteckdosen des Types HM-LC-Sw1-Pl-DN-R1
sowie ergänzend
benutzerdefinierte Geräte mit manuell konfigurierbaren Parametern
unterstützt. 
Die Anbindung anderer Gerätetypen ist bis auf Weiteres nicht geplant.
Bei Interesse können *nach vorheriger Absprache* Pull-Request zur Funktionsweiterung erstellt werden.


## HM-CC-RT-DN: Funk-Heizkörperthermostat

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


## HM-LC-Sw1-Pl-DN-R1: Funk-Zwischenstecker-Schaltaktor 1fach

### Gelesene Werte

| Wert    | Beschreibung            |
|---------|-------------------------|
| STATE   | Aktueller Schaltzustand |
| INHIBIT | Handbedienung gesperrt? |

### Aktionen

* Ein-/Ausschalten des Geräts
* Treppenlicht-Funktion (Einschalten mit automatischem Ausschalten nach eingestellter Einschaltdauer)
* Sofortigen Werteabruf von CCU auslösen (sonst in regelmäßigem Intervall)
* Ein-/Ausschalten der Bediensperre


## Benutzerdefinierte Geräte

Für die Integration beliebiger Homematic-Geräte mit flexibler Auswahl von bis zu 5 Parametern innerhalb eines Geräte-Kanals.

**Anwendungsfälle:**

* Integration nicht explizit unterstützter Gerätetypen
* Zugriff auf spezielle Geräteparameter
* Prototyping neuer Geräteintegrationen
* Erweiterte Konfiguration bestehender Geräte

### Unterstützte Datentypen

| Typ         | KNX DPT                                                                                 |
|-------------|-----------------------------------------------------------------------------------------|
| **action**  | 1.017 (Trigger)                                                                         |
| **boolean** | 1.001 (Schalten)                                                                        |
| **float**   | DPT 9 (16Bit Float)<br />DPT 14 (32Bit Float)<br />DPT5.001 (Prozent)                   |
| **integer** | DPT 13 (32Bit Ganzzahl mit Vorzeichen)<br />DPT 5.005 (0..255)<br />DPT 5.001 (Prozent) |
| **option**  | DPT 13 (32Bit Ganzzahl mit Vorzeichen)<br />DPT 5.005 (0..255)                          |

### Zugriff
 
* Lesen
* Schreiben
* **Bislang nicht unterstützt:** Über Ereignisse

### Aktionen

* Auslösen von bis zu 5 Aktionen durch Schreiben des Parameters
* Sofortigen Werteabruf von CCU auslösen (sonst in regelmäßigem Intervall)


## Event-Verarbeitung (Callback-Architektur)

Das Modul unterstützt Events von der Homematic-CCU über einen HTTP-Webserver-Endpunkt (`POST /HMG/events`).

### Architektur

Events werden von der CCU als XML-RPC-Nachrichten mit den Methodennamen `"event"` (einzeln) oder `"system.multicall"` (gebündelt) gesendet:

```
CCU2  ──XML-RPC event──> Webserver (/HMG/events) ──parse──> Module Event Handler
                                                              ├─> Address: "SERIAL:CHANNEL"
                                                              ├─> value_key
                                                              └─> value (auto-typed)
```

### Datentyp-Erkennung

Empfangene Werte werden automatisch typisiert:

| Erkennung | Typ | Beispiel |
|-----------|-----|---------|
| `"true"` / `"false"` | `bool` | `true` |
| Dezimalzahl ohne `.` | `int32_t` | `42`, `-17` |
| Mit `.` oder E-Notation | `double` | `1.5`, `2e-3` |

### Logging

Jedes empfangene Event wird mit Seriennummer und Kanalnummer geloggt:

```
HMG event: serial=NEQ0123456 channel=1 key=TEMPERATURE value=21.5
HMG event: serial=OEQ5555555 channel=0 key=STATE value=true
```

### Status: Partial Implementation

**Implementiert:**
- XML-RPC Event-Parsing für `"event"` und `"system.multicall"`
- Automatische Typ-Erkennung (bool/int32/double)
- Logging mit Seriennummer + Channel-Nr.
- Module-interne Datentyp-Handler

**Ausstehend:**
- Integration mit `HomematicChannel::processResponseParamX()` zur KO-Aktualisierung
- Geräte-Registrierungs-Tabelle (Serial→Channel-Lookup)
- Reachability-Update (UNREACH-Zustand auf Event-Empfang zurücksetzen)

Detaillierte Implementierungspläne: siehe [doc/CONCEPT-Events.md](doc/CONCEPT-Events.md)

