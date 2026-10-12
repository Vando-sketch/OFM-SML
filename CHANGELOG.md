# Changes

## 1.2.0

- Neu: OBIS-ASCII nach IEC 62056-21 im Push-Betrieb (z. B. Logarex LK13BE). Pro Kanal einstellbar über die neuen Parameter Protokoll, Baudrate, Datenformat und Empfangs-Timeout. Die ASCII-Werte laufen durch dieselbe Auswertung wie SML, Kommunikationsobjekte, Sendeverhalten, MQTT und `sml info` sind daher identisch.
- Neu: Kommunikationsobjekt "Zähler empfängt" (DPT 1.002) pro Kanal, sendet bei Änderung.
- Neu: `sml info` zeigt auf der Konsole zusätzlich Protokoll, Baudrate und Datenformat, z. B. `ASCII 9600 7E1 1R T0 P`. Das Diagnose-KO antwortet unverändert kurz (DPT 16, 14 Zeichen).
- Neu: `sml debug` gibt im ASCII-Betrieb jede Rohzeile, die Kennungszeile und die Zahl verworfener Zeilen aus. Im SML-Betrieb erscheint einmalig ein Hinweis, wenn die empfangenen Daten nach OBIS-ASCII aussehen.
- Geändert: Der Empfangs-Timeout für Status-LED und Statusauswertung ist jetzt pro Kanal einstellbar (bisher fest 5 Sekunden).
- Geändert: Kanäle ohne serielle Schnittstelle werten ihren Status trotzdem aus und melden beim Start einmalig einen Hinweis.
- ⚠️ Der Parameterblock pro Kanal wächst von 28 auf 32 Byte und jeder Kanal bekommt ein weiteres Kommunikationsobjekt. Nach dem Update ist ein vollständiger Download in der ETS nötig.

## 1.1.0

- Fix: Der Parameter "Zählerstände (Wh)" wurde nie ausgewertet (Offset-Makro
  statt `Param...` verwendet), die Wh-Kommunikationsobjekte wurden dadurch
  unabhängig von der Einstellung beschrieben.
- Fix: Die Wh-Zählerstände hingen am Parameter der kWh-Zählerstände — nur Wh
  aktiviert bedeutete, dass gar nichts geschrieben wurde. Beide Varianten sind
  jetzt wie in der ETS dargestellt unabhängig schaltbar.
- Fix: Bei Zweirichtungszählern mit Tarif fehlten die Kommunikationsobjekte für
  die Einspeisung in Tarif 1/2 (kWh und Wh) in der ETS, obwohl die Firmware sie
  befüllt hat. Zusätzlich waren deren Einheiten in den KO-Texten als "kW" statt
  "Wh" ausgewiesen.

## 1.0.0

Erste offizielle Veröffentlichung. Die komplette Basisimplementierung des
Moduls ist Teil dieser Version, deshalb gibt es keine separaten
Änderungseinträge.

Vor dieser Veröffentlichung gab es mehrere Testversionen. Diese sind hier
nicht aufgeführt, eine Änderungshistorie dazu existiert nicht.
