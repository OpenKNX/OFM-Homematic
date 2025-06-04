<!-- SPDX-License-Identifier: AGPL-3.0-only -->
<!-- Copyright (C) 2025 Cornelius Köpp -->
# Applikationsbeschreibung OFM-Homematic



# Inhaltsverzeichnis

* ETS-Konfiguration:<br />
  [**HomeMatic**](#ets-applikationsteilhomematic)
    * [**Allgemein**](#allgemein)
      * [CCU](#ccu)
      * [Geräte-Kommunikation](#geräte-kommunikation)
    * [**Gerät n: ...**](#gerät-n-)
    <!-- * [**(mehr)**](#mehr) -->

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

#### Verzögerung nach Neustart

Legt fest wie lange nach einem Neustart des Gerätes gewartet werden soll
bis versucht wird mit der CCU zu kommunizieren.

***Hinweis:*** 
Der Zeitraum sollte (in Kombination mit der Startverzögerung des Gerätes) so lang gewählt werden, 
dass der Start der CCU abgeschlossen ist. 
Ansonsten würde z.B. bei Neustart nach Stromausfall fälschlicherweise ein Kommunikationsfehler erkannt werden.  


### CCU

Einstellungen für die Verbindung mit der HomeMatic-CCU, oder kompatiblem Gerät.

#### Host

Die IP-Adresse oder den Hostnamen fest.

**Wichtig:** Diese Einstellung ist zwingend erforderlich für die Kommunikation mit den Homematic-Geräten. 


#### Port

Erlaubt die Auswahl des Netzwerkport zum Verbindungsaufbau mit der CCU.
Der Standard-Port 2001 ist voreingestellt.


### Geräte-Kommunikation

Hinweis: Die aktuelle Implementation erlaubt ausschließlich einen **Zyklischen Datenabruf**.
D.h.: Das Gateway-Modul wird regelmäßig die aktuellen Werte der Geräte von der CCU abfragen.  

#### Update-Intervall

Legt fest nach welchem Zeitraum (in Sekunden) eine automatische Aktualisierung der Daten erfolgt.

Der Zeitraum beginn bei Aktualisierungen, auch solchen die durch andere Ereignisse erfolgen, jeweils von Neuem. 
 

#### Update-Intervall kurz (nach Schreiben)

Nach dem Auslösen von Geräte-Aktionen steht der resultierende Status nicht sofort zur Verfügung.
Daher wird anschließend einmalige eine schnelle Aktualisierung geplant.
Der Zeitraum (in Sekunden) sollte kürzer definiert sein als das normale **Update-Intervall**.

### Zusätzliche Kanäle
Dieser Bereich wird sichtbar, wenn mehr Kanäle verfügbar sind als aktuell ausgewählt. Hiermit können zusätzliche HomeMatic-Kanäle zur Konfiguration hinzugefügt werden.


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
welcher Typ von Gerät durch diesen Kanal abgebildet werden soll.

* **inaktiv**
* **HM-CC-RT-DN (Funk-Thermostat)**

Die Sichtbarkeit der nachfolgenden Parameter ist abhängig von der Art des Gerätes.  

> Bei Einstellung *inaktiv* sind alle nachfolgenden Konfigurationen ausgeblendet.



<!-- DOC -->
### Kanal deaktivieren (zu Testzwecken)

Sorgt dafür, dass der Kanal wie mit Geräte-Typ **inaktiv** behandelt wird, 
lässt allerdings die Konfiguration in der ETS sichtbar und erhält die GA-Verknüpfungen. 

<!-- DOC -->
### Seriennummer

Die eindeutige Homematic-Seriennummer


<!-- DOC -->
#### Steuerung über KNX erlauben

Legt fest, ob der Zustand des Gerätes beeinflusst werden kann.
Ansonsten können ausschließlich Status-Werte abgerufen werden.




# Kommunikationsobjekte

## Je Gerät

|       KO | Geräte-Typ                    |    DPT | Bezeichnung                | Erklärung                   |
|---------:|-------------------------------|-------:|----------------------------|-----------------------------|
|       +0 | **alle**                      |  1.001 | Erreichbar                 | Ausgabe Erreichbar          |
|       +1 | **alle**                      |  1.017 | Abfrage ausführen          | Auslöser Update             |
|       +2 | HM-CC-RT-DN (Funk-Thermostat) |  9.001 | Aktuelle Temperatur        | Ausgabe aktuelle Temperatur |
|       +3 | **alle**                      |  9.020 | Batteriestatus             | Ausgabe Batteriestatus      |
|       +4 | HM-CC-RT-DN (Funk-Thermostat) |  1.011 | Boost-Status               | Ausgabe Boost-Status        |
|       +5 | HM-CC-RT-DN (Funk-Thermostat) |  1.017 | Boost starten              | Eingang Boost ausführen     |
|       +6 | **?** alle                    |  1.005 | Fehler                     | Ausgabe Fehler              |
|       +7 | **?** alle                    | 16.001 | Diagnose                   | Ausgabe Diagnose            |
|       +8 | HM-CC-RT-DN (Funk-Thermostat) |  9.001 | Soll-Temperatur            | Eingang Soll-Temperatur     |
|       +9 | HM-CC-RT-DN (Funk-Thermostat) |  9.001 | Soll-Temperatur            | Ausgabe Soll-Temperatur     |
|      +10 | HM-CC-RT-DN (Funk-Thermostat) |  5.001 | Ventilposition             | Ausgabe Ventil-Position     |
|      +11 | **alle**                      |  8.001 | Signal-Qualität            | Ausgabe Signal-Qualität     |

