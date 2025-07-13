
| i  | Type               | Name                                   | KO+0       | KO+1              | KO+2      | KO+3   | KO+4            | KO+5            | KO+6                | KO+7                | KO+8                | KO+9                | KO+10               | KO+11          | KO+12 |
|----|--------------------|----------------------------------------|------------|-------------------|-----------|--------|-----------------|-----------------|---------------------|---------------------|---------------------|---------------------|---------------------|----------------|-------|
| 0  | -                  | inaktiv                                | -          | -                 | -         | -      | -               | -               | -                   | -                   | -                   | -                   | -                   | -              | -     |
| 1  | HM-CC-RT-DN        | Funk-Thermostat                        | Erreichbar | Abfrage ausführen | Diagnose  | Fehler | Signal-Qualität | Batteriestatus  | Aktuelle Temperatur | Soll-Temperatur [A] | Soll-Temperatur [E] | Boost-Status        | Boost starten       | Ventilposition |       |
|    |                    |                                        |            |                   |           |        |                 |                 | DPST-9-1            | DPST-9-1            | DPST-9-1            | DPST-1-11           | DPST-1-17           | DPST-5-1       |       |
| 6  | HM-LC-Sw1-Pl-DN-R1 | Funk-Zwischenstecker-Schaltaktor 1fach | Erreichbar | Abfrage ausführen | Diagnose  | Fehler | Signal-Qualität | -               | Schalt-Status       | Schalt-Befehl       | Treppenhaus         | Sperre-Handbed. [A] | Sperre-Handbed. [E] |                |       |
|    |                    |                                        |            |                   |           |        |                 |                 | DPST-1-11           | DPST-1-1            | DPST-1-1            | DPST-1-11           | DPST-1-1            |                |       |
| KO |                    |                                        | ~Reachable | ~TriggerRequest   | ~Diagnose | ~Error | ~SignalQuality  | ~BatteryVoltage | ~__KO__06           | ~__KO__07           | ~__KO__08           | ~__KO__09           | ~__KO__10           | ~__KO__11      |       |
| Id |                    |                                        | 000        | 001               | 007       | 006    | 011             | 003             | 002                 | 010                 | 008                 | 004                 | 005                 | 009            |       |


KOd%C%
KOd%C%Diagnose
KOd%C%Error
KOd%C%SignalQuality
KOd%C%BatteryVoltage
KOd%C%TempCurrent
KOd%C%TempSetCurrent
KOd%C%TempSet
KOd%C%BoostState
KOd%C%BoostTrigger
KOd%C%ValveState

