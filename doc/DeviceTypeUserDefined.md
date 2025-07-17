# Konfigurationsseite

```
... Standardparameter (ohne Steuern erlauben)...

Batteriebetrieben               (*) ja   (*) nein
Homematic-Geräte-Kanalnummer    [0..255]

## Geräte-Datenpunkt 1..5

Kommentar                       [            ]
Typ                             [          |v]  // {-, action, boolean, float, integer, option}
Parameter-Name in ParamSet      [            ]  // 31 Zeichen
Zugriff                         [          |v]  // {-,lesend}x{-,schreibend}x{-,über Ereignisse}
```

### Später
```
Erweiterte Einstellungen:
  Einheit                       [            ]  // nur float, integer
  Minimalwert                   [          ]^v  // nur float, integer, option
  Maximalwert                   [          ]^v  // nur float, integer, option
```



