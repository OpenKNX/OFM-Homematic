<!-- SPDX-License-Identifier: AGPL-3.0-only -->
<!-- Copyright (C) 2025 Cornelius Köpp -->
# Applikationsbeschreibung OFM-Homematic



# Inhaltsverzeichnis

* ETS-Konfiguration:<br />
  [**HomeMatic**](#ets-applikationsteilhomematic)
    * [**Allgemein**](#allgemein)
      * [CCU](#ccu)
      * [Geräte-Kommunikation](#geräte-kommunikation)
    * [**Geräteübersicht**](#geräteübersicht)
    * [**Gerät n: ...**](#gerät-n-)
    * [**(mehr)**](#mehr)

* [Kommunikationsobjekte](#kommunikationsobjekte)






# ETS-Applikationsteil:<br />HomeMatic


## Allgemein

### Modul-Version

Zeigt die Version (X.Y) des Moduls OFM-DFA an.
Diese Version folgt dem Versionierungsschema der ETS und nicht dem Prinzip der Semantischen Versionierung (SemVer).


<!-- DOC -->
### Kanalauswahl

#### Verfügbare Kanäle

Je nach Applikation kann eine größere Anzahl von Kanälen vorhanden sein,
wobei oft nur einige wenige benötigt werden.
Mit diesem Parameter können nicht benötigt Kanäle in der ETS ausgeblendet werden, bzw. nur die notwendigen angezeigt.

***Bemerkung:***
Ausgeblendete Kanäle sind inaktiv und zugehörige KOs sind ausgeblendet.


### Startverhalten

<!-- DOC -->
#### Verzögerung nach Neustart

Legt fest wie lange nach einem Neustart des Gerätes gewartet werden soll
bis versucht wird mit der CCU zu kommunizieren.

***Hinweis:*** 
Der Zeitraum sollte (in Kombination mit der Startverzögerung des Gerätes) so lang gewählt werden, 
dass der Start der CCU abgeschlossen ist. 
Ansonsten würde z.B. bei Neustart nach Stromausfall fälschlicherweise ein Kommunikationsfehler erkannt werden.  


### CCU

Einstellungen für die Verbindung mit der Homematic-CCU, oder kompatiblem Gerät.

<!-- DOC -->
#### Host

Gibt die IP-Adresse oder den Hostnamen der Homematic-CCU an.

**Wichtig:** Diese Einstellung ist zwingend erforderlich für die Kommunikation mit den Homematic-Geräten. 


<!-- DOC -->
#### Port

Erlaubt die Auswahl des Netzwerkport zum Verbindungsaufbau mit der CCU.
Der Standard-Port 2001 ist voreingestellt.


### Geräte-Kommunikation

Hinweis: Die aktuelle Implementation erlaubt ausschließlich einen **Zyklischen Datenabruf**.
D.h.: Das Gateway-Modul wird regelmäßig die aktuellen Werte der Geräte von der CCU abfragen.  

<!-- DOC -->
#### Update-Intervall

Legt fest nach welchem Zeitraum (in Sekunden) eine automatische Aktualisierung der Daten erfolgt.

Der Zeitraum beginn bei Aktualisierungen, auch solchen die durch andere Ereignisse erfolgen, jeweils von Neuem. 

<!-- DOC -->
#### Update-Intervall kurz (nach Schreiben)

Nach dem Auslösen von Geräte-Aktionen steht der resultierende Status nicht sofort zur Verfügung.
Daher wird anschließend einmalige eine schnelle Aktualisierung geplant.
Der Zeitraum (in Sekunden) sollte kürzer definiert sein als das normale **Update-Intervall**.


<!-- DOC -->
## Geräteübersicht

### Bekannte Geräte

Über den Button *Bekannte Geräte ermitteln* kann von der CCU eine Liste der bekannten/eingerichteten Geräte abgerufen und im Feld *Bekannte Geräte* angezeigt werden.

### Tabelle Geräteübersicht

Die Tabelle liefert eine Alternative zur Anzeige und Einrichtung von Homematic-Geräten.

Die Spalten stellen die wichtigste Teilmenge der Geräte-spezifischen Parameter dar.

Spalten:

* **Name**: Sprechende Bezeichnung für das Gerät. Wird nur in der ETS verwendet.
* **Geräte-Typ**: Legt fest, ob das Gerät aktiv ist und wenn ja, um welchen Typ von Gerät es sich handelt.
* **!**: Temporäres deaktivieren des Gerätes (unter beibehaltung aller Konfigurationswerte und GA-Verknüpfungen).
* **Seriennr.**: Die Homematic-Seriennummer zur Identifikation des Gerätes.
* **1** bis **5**: Zuordnung des Gerätes zur Aggregationsgruppe 1 bis 5.

> ***Hinweis:*** Eine vollständige Konfiguration kann über die jeweils Seite *Gerät n* erfolgen.

## Gerät n: ...

### Geräte-Definition


<!-- DOC -->
#### Name / Beschreibung

Hier sollte zur Dokumentation eine individuelle und eindeutige kurze sprechend Bezeichnung des Geräts hinterlegt werden.
Der Wert wird ausschließlich in der ETS verwendet (als Teil der Kanalbezeichnung) und hat keinen Einfluss auf das Geräteverhalten.


<!-- DOC -->
#### Kommentar

Hier kann eine ausführlichere - auch mehrzeilige - Dokumention zum Gerät erfolgen (z.B. Installationsort, Abhängigkeiten).
Die Dokumentation hat keinen Einfluss auf das Geräteverhalten.

Eine mehrzeilige Eingabe ist aufgrund von Beschränkungen der ETS nicht direkt möglich,
kann jedoch durch Eingabe mit `\n` und Drücken des Buttons erzeugt werden.
Anschließend kann der Text mehrzeilig bearbeitet werden.

<!-- DOCEND -->
> Beispiel:
>
> Der Text `Ein Text\nmit mehreren\nZeilen!` wird umgewandelt in
> ```
> Ein Text
> mit mehreren
> Zeilen!
> ```


<!-- DOC -->
#### Geräte-Typ

Legt fest, ob der Kanal aktiv ist und wenn ja, 
welcher Typ von Gerät durch diesen Kanal abgebildet werden soll:

* **inaktiv**
* **benutzerdefiniert       (für nicht aufgeführte Geräte)**
* **HM-CC-RT-DN             (Funk-Thermostat)**
* **HM-LC-Sw1-Pl-DN-R1      (Funk-Zwischenstecker-Schaltaktor 1fach)**

Die Sichtbarkeit der nachfolgenden Parameter ist abhängig von der Art des Gerätes.  

> Bei Einstellung *inaktiv* sind alle nachfolgenden Konfigurationen ausgeblendet.



<!-- DOC -->
#### Kanal deaktivieren (zu Testzwecken)

Sorgt dafür, dass der Kanal wie mit Geräte-Typ **inaktiv** behandelt wird, 
lässt allerdings die Konfiguration in der ETS sichtbar und erhält die GA-Verknüpfungen. 

<!-- DOC -->
#### Seriennummer

Die eindeutige Homematic-Seriennummer


### Konfiguration


<!-- DOC -->
#### Steuerung über KNX erlauben

<!-- DOC Skip="2" -->
> Die Option wird für den Geräte-Typ benutzerdefiniert nicht angeboten.

Legt fest, ob der Zustand des Gerätes beeinflusst werden kann.
Ansonsten können ausschließlich Status-Werte abgerufen werden.


<!-- DOC -->
#### Einschaltdauer

<!-- DOC Skip="2" -->
> Die Option wird nur für Geräte mit Typ Schaltaktor (mit Treppenhausfunktion) angeboten.

Legt fest, wie nach welcher Verzögerung wieder ausgeschaltet wird bei Nutzung der Treppenhaus-Funktion


<!-- DOC -->
#### Batteriebetrieben

<!-- DOC Skip="2" -->
> Die Option wird nur für den Geräte-Typ benutzerdefiniert angeboten.

Für Geräte mit Stromversorgung per Batterie wird ein KO zur Ausgabe des Batteriestatus bereitgestellt. 


### Datenpunkt-Konfiguration

> Diese Einstellungen werden nur für den Geräte-Typ *benutzerdefiniert* angeboten.



<!-- DOC -->
#### Homematic-Geräte-Kanalnummer

Legt fest zu welcher Kanal-Nummer des Gerätes die nachfolgend definierten Datenpunkte gehören.

> ***Hinweis:*** Falls Datenpunkte aus mehreren Kanalnummern genutzt werden sollen, so können mehrere benutzerdefinierte Instanzen mit derselben Seriennummer angelegt werden.


#### Datenpunkt 1 bis 5


<!-- DOC -->
##### Parameter-Name

Hier muss der exakte technische Parameter-Name angegeben werden.
Dieser ist der Dokumentation der Geräte-Datenpunkte von Homematic zu entnehmen, 
oder ist alternativ selbst zu ermitteln über Abfrage aus der CCU. 


<!-- DOC -->
##### Typ (und Abbildung in KNX)

Hier muss der Datentyp des Datenpunktes angegeben werden.
Bei einigen Datentypen muss gleichzeit auch ausgewählt werden wie diese in KNX dargestellt werden sollen, 
bzw. abhängig vom genutzten Wertebereich (siehe Dokumentation der Geräte-Datenpunkte von Homematic) können.

Bei Auswahl **-** wird dieser Datenpunkt deaktiviert.

> ***Hinweis:*** Weitere KNX-DPTs können ggf. unter Einsatz eines Kanals des OpenKNX-Logikmoduls realisiert werden.


<!-- DOC -->
##### Zugriff

Hier muss angegeben werden in welcher Form auf den Datenpunkt zugegriffen werden kann und soll.
Es existieren drei Zugriffsarten auf Homematic-Datenpunkte:

* **lesend** (Read):
  Stellt den vom Gerät (bei Abfrage) zurückgemeldeten Wert über ein Status-KO bereit. 
* **schreibend** (Write):
  Stellt ein KO bereit überschreiben des Wertes im Gerät.
* **über Ereignisse** (Event): 
  ***(- bislang nicht unterstützt -)***
  Benachrichtigt sofort (ohne explizite Abfrage) bei Änderung im Gerät.
  Der Wert wird wie bei *lesend* über ein Status-KO bereitgestellt. 

Es kann eine Kombination von Zugriffsarten (Teilmenge aus Dokumentation der Geräte-Datenpunkte von Homematic) gewählt werden. 

Bei Auswahl **-** wird dieser Datenpunkt deaktiviert.


<!-- DOC -->
#### Zuordnung zu Aggregationsgruppen

Das Gerät kann den Gruppen 1 bis 5 zugeordnet werden.
Für jede dieser Gruppen werden folgende Status-Werte (auf Basis aller enthaltenen Geräte) ermittelt:

* Kommunikationsstörung
* Batteriewarnung
* Fehler

*Hinweis:* Zusätzlich erfolgt auch eine globale Aggregation mit allen Geräten unabhängig von einer Gruppenzuordnung. 


## (mehr)

Dieser Bereich wird sichtbar, wenn mehr Kanäle verfügbar sind als aktuell ausgewählt.
Hiermit können zusätzliche HomeMatic-Kanäle zur Konfiguration hinzugefügt werden.


# Kommunikationsobjekte

## Globale Statuswerte

|           KO | Aggregation           |    DPT | Bezeichnung           | Erklärung                                                                                                                     |
|-------------:|-----------------------|-------:|-----------------------|-------------------------------------------------------------------------------------------------------------------------------|
|           +0 | Global                |  1.005 | Kommunikationsstörung | Gibt einen Alarm aus, wenn eines mindestens eines der Geräte nicht erreichbar ist.                                            |
|           +1 | Global                |  1.005 | Batteriewarnung       | Gibt einen Alarm aus, wenn für mindestens eines der (batteriebetriebenen) Geräte vor einer bald leeren Batterie gewarnt wird. |
|           +2 | Global                |  1.005 | Fehler                | Gibt einen Alarm aus, wenn für mindestens eines der Geräte ein Fehler vorliegt.                                               |
| +3*i&nbsp;+0 | Gruppe i<br>(1 bis 5) |  1.005 | Kommunikationsstörung | Analog globaler Kommunikationsstörung, aber beschränkt auf Geräte aus Gruppe i.                                               |
| +3*i&nbsp;+1 | Gruppe i<br>(1 bis 5) |  1.005 | Batteriewarnung       | Analog globaler Batteriewarnung, aber beschränkt auf Geräte aus Gruppe i.                                                     |
| +3*i&nbsp;+2 | Gruppe i<br>(1 bis 5) |  1.005 | Fehler                | Analog globaler Fehler, aber beschränkt auf Geräte aus Gruppe i.                                                              |

## Je Gerät

|  KO | Geräte-Typ                          |          DPT | Bezeichnung                  | Erklärung                                       |
|----:|-------------------------------------|-------------:|------------------------------|-------------------------------------------------|
|  +0 | **alle**                            |        1.001 | Erreichbar                   | Ausgabe Erreichbar                              |
|  +1 | **alle**                            |        1.017 | Abfrage ausführen            | Auslöser Update                                 |
|  +2 | **alle**                            |       16.001 | Diagnose                     | Ausgabe Diagnose                                |
|  +3 | **alle**                            |        1.005 | Fehler                       | Ausgabe Fehler                                  |
|  +4 | **alle**                            |        8.001 | Signal-Qualität              | Ausgabe Signal-Qualität                         |
|  +5 | **alle mit Batterie**               |        9.020 | Batteriestatus               | Ausgabe Batteriestatus                          |
|  +6 | HM-CC-RT-DN (Funk-Thermostat)       |        9.001 | Aktuelle Temperatur          | Ausgabe aktuelle Temperatur                     |
|  +7 | HM-CC-RT-DN (Funk-Thermostat)       |        9.001 | Soll-Temperatur              | Ausgabe Soll-Temperatur                         |
|  +8 | HM-CC-RT-DN (Funk-Thermostat)       |        9.001 | Soll-Temperatur setzen       | Eingang Soll-Temperatur                         |
|  +9 | HM-CC-RT-DN (Funk-Thermostat)       |        1.011 | Boost-Status                 | Ausgabe Boost-Status                            |
| +10 | HM-CC-RT-DN (Funk-Thermostat)       |        1.017 | Boost starten                | Eingang Boost ausführen                         |
| +11 | HM-CC-RT-DN (Funk-Thermostat)       |        5.001 | Ventilposition               | Ausgabe Ventil-Position                         |
|  +6 | HM-LC-Sw1-Pl-DN-R1 (Funk-Steckdose) |        1.011 | Schalt-Status                | Ausgabe aktueller Schalt-Status                 |
|  +7 | HM-LC-Sw1-Pl-DN-R1 (Funk-Steckdose) |        1.001 | Schalt-Befehl                | Eingang Schalten                                |
|  +8 | HM-LC-Sw1-Pl-DN-R1 (Funk-Steckdose) |        1.001 | Treppenhaus-Schalt-Befehl    | Eingang Schalten mit Treppenhausfunktion        |
|  +9 | HM-LC-Sw1-Pl-DN-R1 (Funk-Steckdose) |        1.011 | Sperr-Status                 | Ausgabe Sperre aktiv                            |
| +10 | HM-LC-Sw1-Pl-DN-R1 (Funk-Steckdose) |        1.017 | Sperren                      | Eingang zum Setzen und Freigeben der Sperre     |
| +12 | benutzerdefiniert                   | verschiedene | *Parameter-Name* (Wert)      | Ausgang mit Wert des 1. Datenpunkts             |
| +13 | benutzerdefiniert                   |      wie +12 | *Parameter-Name* (schreiben) | Eingang zum Setzen des Wertes im 1. Datenpunkts |
| +14 | benutzerdefiniert                   | verschiedene | *Parameter-Name* (Wert)      | Ausgang mit Wert des 2. Datenpunkts             |
| +15 | benutzerdefiniert                   |      wie +14 | *Parameter-Name* (schreiben) | Eingang zum Setzen des Wertes im 2. Datenpunkts |
| +16 | benutzerdefiniert                   | verschiedene | *Parameter-Name* (Wert)      | Ausgang mit Wert des 3. Datenpunkts             |
| +17 | benutzerdefiniert                   |      wie +16 | *Parameter-Name* (schreiben) | Eingang zum Setzen des Wertes im 3. Datenpunkts |
| +18 | benutzerdefiniert                   | verschiedene | *Parameter-Name* (Wert)      | Ausgang mit Wert des 4. Datenpunkts             |
| +19 | benutzerdefiniert                   |      wie +18 | *Parameter-Name* (schreiben) | Eingang zum Setzen des Wertes im 4. Datenpunkts |
| +20 | benutzerdefiniert                   | verschiedene | *Parameter-Name* (Wert)      | Ausgang mit Wert des 5. Datenpunkts             |
| +21 | benutzerdefiniert                   |      wie +20 | *Parameter-Name* (schreiben) | Eingang zum Setzen des Wertes im 5. Datenpunkts |

