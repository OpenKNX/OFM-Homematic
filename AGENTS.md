# AGENTS für OFM-Homematic

## Ziel

Das Modul ermöglicht lesenden und schreibenden Zugriff auf Homematic-Geräte (Funkschnittstelle),
über eine Zentrale mit XML-RPC-Schnittstelle wie z.B. der Homematic CCU2.
Als Gateway zwischen KNX und Homematic können Status-Werte auf dem KNX-Bus bereitgestellt und Steuerbefehle an Homematic-Geräte gesendet werden. 

## Prefix und Namenskonventionen

- **Modulprefix**: `HMG`
- **C++-Klassen**: `HomematicModule`, `HomematicChannel*`, `Rpc*`
- **ETS-Anzeigename**: `HomematicModule`, **Icon**: `crop-square--square-medium`

## XML-Konventionen

| Platzhalter | Bedeutung                                 |
|-------------|-------------------------------------------|
| `%TT%`      | Modultyp (zweistellig), analog OFM-SML    |
| `%CC%`      | Kanalnummer (zweistellig), analog OFM-SML |

Parameter-IDs: `%AID%_UP-%TT%%CC%NNN` bzw. `%AID%_P-%TT%%CC%NNN` (NNN = dreistellige Nummer).
**Nicht** das NetworkBridge-Schema `%T%%CCC%` verwenden.

## Kanalanzahl

Die Kanalanzahl wird durch das OAM über `HMG_ChannelCount` vorgegeben und ist aus technischen Gründen auf 64 Kanäle beschränkt.

## Gerätemodell und Entwicklungsregeln

### Gerätetypen

- Typ 0 = Inaktiv: Kanal wird als `HomematicChannelInactive` angelegt, keine aktive Kommunikation.
- Typ 1 = `HM-CC-RT-DN` (Thermostat): Soll/Ist-Temperatur, Boost, Ventilzustand.
- Typ 6 = `HM-LC-Sw1-Pl-DN-R1` (Schaltaktor): Schalten, Treppenlicht, Bediensperre.
- Typ 7 = Benutzerdefiniert: bis zu 5 frei konfigurierbare Datapoints (Typ/Zugriff/ParamName).

Seriennummern werden je Kanal über `ParamHMG_dDeviceSerial` konfiguriert.
Ein Kanal gilt nur dann als aktiv, wenn Gerätetyp != 0, Kanal nicht deaktiviert und Seriennummer gesetzt ist.

### Kanal- und Klassenarchitektur

1. `HomematicModule` erzeugt Kanäle ausschließlich über `createChannel()`.
2. Gerätetypspezifische Logik liegt nur in den Ableitungen:
	`HomematicChannelThermostat`, `HomematicChannelSwitchActuator`, `HomematicChannelUserDefined`.
3. `HomematicChannel` kapselt den gemeinsamen Ablauf:
	Polling, Trigger-Request-KO, XML-RPC Request/Response Parsing und Basiszustände.
4. Schreibzugriffe erfolgen über die zentralen Helfer `rpcSetValueBool`, `rpcSetValueDouble`, `rpcSetValueInteger4`.
5. Unbekannte oder nicht unterstützte Gerätetypen werden defensiv auf `HomematicChannelInactive` zurückgeführt.

### Statusaggregation und Gruppen

Das Modul führt je Kanal einen Zustandsbit pro Kategorie:
`unknown`, `unreach`, `batteryWarn`, `error`.

Die Aggregation erfolgt in 6 Gruppenmasken (`_groups[0..5]`):
- Gruppe 0 = alle aktiven Kanäle (global)
- Gruppe 1..5 = konfigurierbare Teilgruppen pro Kanal

Regel für KO-Senden:
- Negative Zustände (`unreach`, `batteryWarn`, `error`) werden sofort gesendet.
- Positive Entwarnung wird erst gesendet, wenn für die jeweilige Gruppe alle Geräte bekannt sind.

### XML-RPC und Parameterauswertung

Pro Update werden `getParamset(..., VALUES)` für Kanal `:0` und den Gerätekanal abgefragt.
Die Verarbeitung erfolgt typisiert über `double`, `i4`, `boolean`.

Basisklasse verarbeitet kanalübergreifend u.a.:
- `RSSI_DEVICE`, `RSSI_PEER`
- `UNREACH`
- `LOWBAT` / `LOW_BAT`

Gerätespezifische Parameter werden in den jeweiligen Channel-Klassen verarbeitet.

### Netzwerkzugriff

Die Kommunikation zur CCU erfolgt per HTTP-POST mit XML-RPC (Content-Type `text/xml`) über Host und Port aus der ETS-Konfiguration (`ParamHMG_Host`, `ParamHMG_Port`).

Aktuell genutzte XML-RPC-Methoden:
- `getParamset` für zyklischen Datenabruf
- `setValue` für Schreibzugriffe
- `rssiInfo` für Geräte-/Signalübersicht
- `getDeviceDescription` für Geräteinformationen in der ETS-Hilfe

Implementierungsdetails (aktueller Stand):
- Transport ist der Webclient aus OFM-Network (`openknxNetwork.webclient`, benötigt `OPENKNX_WEBCLIENT`), angesprochen über `RpcUtil::sendRequestGetResponseDoc()`
- Verarbeitung ist weiterhin synchron/blockierend: `RpcUtil` stößt den Request an und pumpt `webclient.loop()` in einer Warteschleife, bis `onDone` feuert oder ein Timeout greift (TODO: vollständig asynchrone Verarbeitung)
- Request-Timeout ist auf 2000 ms gesetzt (`OPENKNX_WEBCLIENT_TIMEOUT=2000` in `platformio.custom.ini` des Geräteprojekts)
- Keine HTTP-Keep-Alive-Verbindungen (`Connection: close`, eine Anfrage je Verbindung)
- Kommunikation erfolgt derzeit ohne Authentifizierung

Wichtige Betriebsgrenzen:
- Lange oder fehlerhafte HTTP-Requests können andere Modulabläufe zeitlich beeinflussen.
- Fehlerbehandlung erfolgt defensiv über Rückgabewerte, Logging und Fallbacks (z.B. Kanal inaktiv bei nicht unterstütztem Gerätetyp).

### Function-Property für ETS-Hilfsfunktionen

Verwendet wird Objekt 160, Property 7.

Unterstützte Aufrufe:
1. `ScanResult` (0): bekannte Geräte/Seriennummern liefern
2. `DeviceInfo` (1): Details zu einem gescannten Gerät liefern
3. `LastError` (254): letzten Modulfehler ausgeben

Die Rückgabe muss robust gegen ungültige Längen, Indizes und Null-Pointer sein.

### Regeln für Weiterentwicklung

1. Neue Homematic-Gerätetypen erhalten eine eigene Channel-Klasse plus `createChannel()`-Zweig.
2. Keine Gerätetyp-Verzweigungen in `HomematicModule::loop()` oder `processInputKo()`.
3. `HomematicModule` bleibt Orchestrierung: Channel-Lifecycle, Gruppenaggregation, Function-Property.
4. Polling bleibt nicht-blockierend im Sinne der Modulstruktur (kein `delay()`), Zeitsteuerung nur über `millis()`-Logik.
5. Öffentliche ETS-Sicht (Parameter/KOs/HelpContext) und C++-Implementierung müssen synchron geändert werden.
6. Technische Dokumentation in `AGENTS.md` (diese Datei) und `README.md` muss aktuell gehalten werden


## Dokumentation und Hilfe

- Dokumentation liegt in `doc/Applikationsbeschreibung-Homematic.md`
- Jede sichtbare `ParameterRefRef` bekommt einen `HelpContext` mit Prefix `HMG-`
- Jede verwendete `HelpContext`-Id muss in der Applikationsbeschreibung als `<!-- DOC HelpContext="HMG-..." -->` dokumentiert sein
- Baggages werden über VS Code Task "OpenKNXproducer Documentation" erzeugt (`.vscode/tasks.json`)
- Deutsche Texte mit echten Umlauten (ä, ö, ü, ß) schreiben
