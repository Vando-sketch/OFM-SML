# OFM-SML

Liest SML-Telegramme (Smart Message Language) oder OBIS-ASCII-Telegramme (IEC 62056-21, Push-Betrieb) von Stromzählern über einen Optokoppler bzw. eine serielle Schnittstelle und sendet die Zählerstände, Momentanwerte und optional das Statuswort (nur SML) auf den KNX-Bus.

## Funktionsumfang

Unterstützt werden Einrichtungs- und Zweirichtungszähler, jeweils mit oder ohne Tarifumschaltung (HT/NT). Bist du dir beim Zählertyp unsicher, kannst du zunächst Zweirichtungszähler mit Zweitarif wählen — dann werden alle möglichen Zählerstände ausgewertet, unabhängig davon, was der angeschlossene Zähler tatsächlich unterstützt; nicht unterstützte Werte bleiben einfach auf 0 stehen.

Pro Kanal werden bereitgestellt:

- **Zählerstände** – Bezug (OBIS 1.8.0), bei Zweitarifzählern zusätzlich Hoch-/Niedertarif, bei Zweirichtungszählern zusätzlich Einspeisung (2.8.0). Für jeden Zählerstand einzeln konfigurierbar: Änderung und/oder fester Zyklus, optional eine Wh-genaue Variante für Logikmodul/virtuellen Zähler.
- **Momentanwerte** – Wirkleistung, Spannung, Strom, Frequenz, je nachdem was der Zähler liefert.
- **Statuswort** – wird roh übertragen, da die Bit-Belegung herstellerabhängig und nicht verbindlich spezifiziert ist. Details und Beispiele zur Auswertung mit dem Logikmodul stehen in [doc/Applikationsbeschreibung-SML.md](doc/Applikationsbeschreibung-SML.md).

## MQTT

Ist im Netzwerkmodul MQTT aktiviert, veröffentlicht jeder Kanal zusätzlich pro empfangenem Telegramm ein JSON-Objekt mit allen darin enthaltenen Messwerten (z. B. `energy_in`, `power`, `current_l1`, `voltage_l1`, `frequency`, `status`) unter dem Topic `openknx/<geräte-prefix>/sml/<identifikationsnummer>`. Liefert der Zähler keine Identifikationsnummer, wird stattdessen der Kanalbuchstabe verwendet (`.../sml/a`, `.../sml/b`, …). Einen eigenen ETS-Parameter gibt es dafür nicht — die Veröffentlichung folgt automatisch der globalen MQTT-Einstellung im Netzwerkmodul. Details in [doc/Applikationsbeschreibung-SML.md](doc/Applikationsbeschreibung-SML.md).

## Status-LEDs

Modul und jeder Kanal können eine Status-LED anzeigen, nach dem Zustandsmodell der OpenKNX-Wiki-Seite "Status-LED": Farb-LEDs sind rot und blinken bei jedem empfangenen Byte, solange kein gültiges Telegramm erkannt wurde. Einfarbige LEDs bleiben bis zum ersten gültigen Telegramm aus, das Blinken taugt bei ihnen also nicht als Verdrahtungstest. Bei gültigem Empfang leuchtet die LED (grün) und flackert mit den empfangenen Daten. Kommt innerhalb des Empfangs-Timeouts kein gültiges Telegramm mehr, wird sie wieder rot bzw. erlischt.

## Diagnose

Über das Diagnose-Kommunikationsobjekt lässt sich live abfragen, was der angeschlossene Zähler tatsächlich sendet — unabhängig von der aktuellen ETS-Parametrierung. Dazu den Text `sml infoXX` an das Diagnose-KO senden (XX = zweistellige Kanalnummer, z. B. `sml info01`). Die Antwort basiert auf dem zuletzt empfangenen Telegramm:

- **NODATA** – seit mehreren Sekunden ist kein gültiges Telegramm mehr angekommen
- **INACTIVE** – der Kanal ist in ETS nicht aktiviert
- **1R** / **2R** – Einrichtungs- bzw. Zweirichtungszähler
- **T0** / **T2** – keine Tarife (Eintarifzähler) bzw. zwei Tarife (Hoch-/Niedertarif)
- **P**, **V**, **A**, **F** – der Zähler liefert zusätzlich Wirkleistung, Spannung, Strom bzw. Frequenz

So lässt sich schon vor dem Einschalten der einzelnen Funktionen live prüfen, was der angeschlossene Zähler überhaupt kann.

## Konsolenbefehle

- **sml debug** – schaltet eine fortlaufende Anzeige aller empfangenen SML-Daten ein bzw. aus
- **sml infoXX** – zeigt denselben Status wie das Diagnose-KO direkt auf der Konsole, davor Protokoll, Baudrate und Datenformat (z. B. `ASCII 9600 7E1 1R T0 P`). Das Diagnose-KO selbst antwortet ohne diese Angaben, ein DPT-16-Text fasst nur 14 Zeichen
