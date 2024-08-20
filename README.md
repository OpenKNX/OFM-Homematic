# Homematic-Gateway zur Integration in OpenKNX


Von Cornelius Köpp 2024



# Beschreibung
Das Modul ermöglicht einen rudimentären lesenden und schreibenden Zugriff auf ausgewählte Homematic-Geräte (Funkschnittstelle), 
über eine Zentrale mit XML-RPC-Schnittstelle wie z.B. der Homematic CCU2.

# Unterstützte Geräte

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




