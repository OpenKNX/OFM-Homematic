### Zugriff

Hier muss angegeben werden in welcher Form auf den Datenpunkt zugegriffen werden kann und soll.
Es existieren drei Zugriffsarten auf Homematic-Datenpunkte:

* **lesend** (Read):
  Stellt den vom Gerät (bei Abfrage) zurückgemeldeten Wert über ein Status-KO bereit. 
* **schreibend** (Write):
  Stellt ein KO bereit überschreiben des Wertes im Gerät.
* **über Ereignisse** (Event): 
  Benachrichtigt sofort (ohne explizite Abfrage) bei Änderung im Gerät.
  Der Wert wird wie bei *lesend* über ein Status-KO bereitgestellt. 

Es kann eine Kombination von Zugriffsarten (Teilmenge aus Dokumentation der Geräte-Datenpunkte von Homematic) gewählt werden. 

Bei Auswahl **-** wird dieser Datenpunkt deaktiviert.


