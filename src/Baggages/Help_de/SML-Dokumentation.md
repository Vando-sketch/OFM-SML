### Dokumentation


Eine vollständige Applikationsbeschreibung ist unter folgendem Link verfügbar: https://github.com/openknx/OFM-SML/blob/v1/doc/Applikationsbeschreibung-SML.md

Das Modul ermöglicht das Auslesen von Stromzählern in Deutschland, die per Optokoppler SML oder OBIS-ASCII (IEC 62056-21, Push-Betrieb) senden.

Die Zähler selbst arbeiten alle nach demselben Prinzip und unterscheiden sich nur darin, welche Werte sie tatsächlich liefern. Bist du dir beim Zählertyp unsicher, kannst du zunächst Zweirichtungszähler mit Zweitarif (2R, HT/NT) wählen — dann werden alle möglichen Zählerstände ausgewertet, unabhängig davon, was der angeschlossene Zähler tatsächlich unterstützt.

Über die Gerätekonsole lässt sich der Empfang live beobachten:

- **sml debug** – schaltet eine fortlaufende Anzeige aller empfangenen SML-Daten ein bzw. aus, eine erneute Eingabe schaltet sie wieder ab
- **sml infoXX** – zeigt denselben Status wie das Diagnose-Kommunikationsobjekt (siehe unten) direkt auf der Konsole, XX ist die zweistellige Kanalnummer. Auf der Konsole stehen davor Protokoll, Baudrate und Datenformat, zum Beispiel **ASCII 9600 7E1 1R T0 P**

**Diagnose:** Über das Diagnose-Kommunikationsobjekt lässt sich live abfragen, was der angeschlossene Zähler tatsächlich sendet, unabhängig von der aktuellen ETS-Parametrierung. Dazu den Text **sml infoXX** an das Diagnose-KO senden, wobei XX die zweistellige Kanalnummer ist, zum Beispiel **sml info01** für den ersten Kanal. Die Antwort basiert auf dem zuletzt empfangenen Telegramm, zum Beispiel **1R T0 P**:

- **NODATA** – seit Ablauf des Empfangs-Timeouts ist kein gültiges Telegramm mehr angekommen
- **INACTIVE** – der Kanal ist in ETS nicht aktiviert
- **1R** / **2R** – Einrichtungs- bzw. Zweirichtungszähler
- **T0** / **T2** – keine Tarife (Eintarifzähler) bzw. zwei Tarife (Hoch-/Niedertarif)
- **P**, **V**, **A**, **F** – der Zähler liefert zusätzlich Wirkleistung, Spannung, Strom bzw. Frequenz

Die Konsolenausgabe von **sml infoXX** zeigt zusätzlich vor dem Status das eingestellte Protokoll (**SML** bzw. **ASCII**) sowie Baudrate und Datenformat, zum Beispiel **ASCII 9600 7E1 1R T0 P** oder **ASCII 9600 7E1 NODATA**. Die Antwort des Diagnose-Kommunikationsobjekts bleibt dagegen kurz und enthält diese Angaben nicht, denn ein Text im Datenpunkttyp 16 fasst nur 14 Zeichen.

So lässt sich schon vor dem Einschalten der einzelnen Funktionen live prüfen, was der angeschlossene Zähler überhaupt kann.
