# AGENTS für OFM-SML

## Ziel

Dieses Modul liest SML-Telegramme (Smart Message Language) von deutschen Smartmetern über eine serielle Schnittstelle (z. B. eHZ-Infrarot-/KNX-Adapter) und sendet die Zählerstände, Momentanwerte und optional das Statuswort auf den KNX-Bus.

## Basis: OGM-Common

Dieses OFM baut auf `../OGM-Common/AGENTS.md` auf (Modulsystem, LED-/
Flash-/Zeit-API, Logging, Embedded-Rahmenbedingungen, Code-Konventionen)
— dort beschriebene Regeln und vorhandene Claude-Skills/Agents (z. B.
das Kanalauswahl-Muster) gelten auch hier.

## Prefix und Namenskonventionen

- **Modulprefix**: `SML`
- **C++-Klassen**: `SMLModule`, `SMLChannel`
- **ETS-Anzeigename**: `Smartmeter (SML)`, **Icon**: `meter-electric-outline`

## Kanalanzahl

Die Kanalanzahl wird durch das OAM über `SML_ChannelCount` vorgegeben. Jeder Kanal ist einer physischen seriellen Schnittstelle zugeordnet — der Serial-Zeiger wird nicht im OFM, sondern vom einbindenden Projekt über `SMLModule::getChannel(i)->setSerial(...)` gesetzt.

**Wichtig für main.cpp-Integration**: Kanäle werden immer vollständig angelegt (kein Lazy-Allocation-Pattern wie bei anderen Modulen), weil `setSerial()` direkt nach `openknx.setup()` auf jeden Kanal aufgerufen wird und `getChannel()` sonst `nullptr` zurückgeben würde. `SMLChannel::setup()` sichert sich selbst über `if (ParamSML_cType > 0)` ab.

## Telegramm-Verarbeitung

- `SMLChannel::writeBuffer()` nimmt Bytes byteweise auf, erkennt Start-/Endsequenz über eine Escape-State-Machine auf dem 4-Byte-Raster des SML-Transportformats.
- `OPENKNX_SML_BUFFER` (Default 1024) begrenzt die Puffergröße pro Kanal. Läuft ein Telegramm über, wird es verworfen, nicht verschoben.
- `SMLChannel::processFile()` übergibt ein komplettes Telegramm an `libsml` und wertet die OBIS-Einträge über `processDataPoint()` aus.
- Innerhalb eines Telegramms gewinnt für das Statuswort-KO der **erste** OBIS-Eintrag mit Statusfeld — die SML-Spec erlaubt grundsätzlich pro Eintrag ein eigenes, potenziell unterschiedliches Statusfeld.

## OBIS-ASCII (IEC 62056-21)

- Pro Kanal wählt der Parameter `c%C%Protocol` zwischen SML (0) und OBIS-ASCII (1). Eine automatische Erkennung, freie OBIS-Codes und Mode-C-Abfrage (`/?!`) sind bewusst nicht vorgesehen.
- `ObisAsciiParser` (`src/ObisAsciiParser.h/.cpp`) ist frei von Arduino-Abhängigkeiten und wird auf dem Host getestet (`test/`). Er rahmt Telegramme byteweise (`/` … `!`, optional STX/ETX/BCC), blendet Bit 7 aus (7E1 als 8N1 gelesen) und zerlegt die Zeilen.
- `writeBuffer()` verzweigt nach Protokoll; der SML-Teil steht unverändert in `writeBufferSml()`. Das fertige ASCII-Telegramm wird wie `_smlBuffer` per Mutex an `processFile()` übergeben.
- Jeder ASCII-Wert endet im bestehenden `processDataPoint(obis, a, b, c, d, e, f, double, statusSuffix)`. Einheiten werden vorher normiert (kWh → Wh, kW → W), damit KOs, MQTT und `sml info` für beide Protokolle identisch arbeiten. Neue OBIS-Codes daher nur dort ergänzen, nicht im Parser.
- Ein Steuerzeichen im laufenden Telegramm verwirft es sofort (`DropReason::Garbage`), damit falsche Baudrate/falsches Format nicht als Pufferüberlauf erscheinen. Diagnose (einmaliger Hinweis, Hex-Dump in `sml debug`) steht in `SMLChannel::writeBufferAscii()`/`logAsciiDrop()`.
- Die Identifikationsnummer (96.1.0) wird um führende Nullen gekürzt, bis sie 14 Zeichen lang ist (DPT 16.001).
- Parameterblock pro Kanal: 32 Byte. Offset 28–29 belegt (Protokoll, Baudrate, Datenformat, Empfangs-Timeout), Offset 30–31 reserviert.

## Zählertypen

Typ 0 = Inaktiv. Typ 1–4 unterscheiden Einrichtungs-/Zweirichtungszähler jeweils mit oder ohne Tarif (HT/NT), siehe `PT-SMLType` in `SMLModule.templ.xml`.

## Statuswort

Das Statuswort wird roh (unausgewertet) als 32-Bit-Wert übertragen — die Bit-Belegung ist herstellerabhängig und nicht verbindlich spezifiziert. Auswertung einzelner Bits ist Aufgabe des Logikmoduls, nicht dieses Moduls. Details und Bit-Tabellen (unverifizierte Sekundärinformation) stehen in `doc/Applikationsbeschreibung-SML.md`.

Das Ü-Flag ist beim Statuswort-KO und den Wh-genauen Zählerstand-Varianten standardmäßig deaktiviert (`TransmitFlag="Disabled"` in `SMLModule.templ.xml`) — diese Werte sind für die geräteinterne Weiterverarbeitung (Logikmodul, virtueller Zähler) gedacht, nicht für die direkte Übertragung per Gruppenadresse.

## MQTT

Pro empfangenem Telegramm veröffentlicht jeder Kanal einen JSON-Snapshot mit allen darin enthaltenen Messwerten unter `sml/<identifikationsnummer>` (Fallback: Kanalbuchstabe `a`, `b`, … wenn der Zähler keine OBIS 96.1.0 liefert) — bewusst ein Objekt statt einer Nachricht pro Wert, damit Abonnenten einen zeitlich konsistenten Stand sehen. Kein Retain, weil der Payload keinen Zeitstempel enthält.

- Ein **eigener ETS-Parameter existiert nicht** — die Veröffentlichung folgt der globalen MQTT-Einstellung des Netzwerkmoduls.
- Der ganze Zweig hängt an `#if (defined(KNX_IP_WIFI) || defined(KNX_IP_LAN)) && defined(OPENKNX_MQTT)`. `mqttBegin()` läuft am Anfang von `processFile()`, `mqttAppend()` in den `processDataPoint()`-Zweigen, `mqttPublish()` am Ende — neue Messwerte gehören genau dorthin, wo auch das zugehörige KO geschrieben wird.
- JSON wird über `OpenKNX::Format::JSON::Writer` (OFM-Network) gebaut, nicht von Hand; der `Writer` ist Member und wird per `reset()` wiederverwendet.

## Status-LEDs

Modul (`SML Gesamtstatus`, Funktions-ID 200) und jeder Kanal (`SML Kanalstatus X`, ID 201+n) folgen dem Zustandsmodell der OpenKNX-Wiki-Seite "Status-LED": Farb-LEDs blinken rot bei jedem empfangenen Byte, solange kein gültiges Telegramm erkannt wurde; einfarbige LEDs bleiben bis zum ersten gültigen Telegramm dunkel (`ledHelper()` in `SMLModule.cpp`). Bei gültigem Empfang leuchtet die LED, nach Ablauf des Empfangs-Timeouts (ETS-Parameter `c%C%ReceiveTimeout`, 1–255 s) ohne gültigen Empfang wird sie rot bzw. erlischt. Dasselbe Timeout steuert das KO "Zähler empfängt". Der Gesamtstatus des Moduls ist aktiv, sobald mindestens ein Kanal aktiv empfängt.

## Konsolenbefehle

- **sml debug** – schaltet ausführliches Logging aller OBIS-Werte und Rohtelegramme ein/aus.
- **sml infoNN** – zeigt den erkannten Zählertyp (1R/2R, höchster Tarif, verfügbare Messgrößen P/V/A/F) für Kanal NN, auch über das Diagnose-KO abrufbar. Nur die Konsole stellt Protokoll, Baudrate und Datenformat voran (`SMLChannel::serialInfo()`, z. B. `ASCII 9600 7E1 1R T0 P`). Das Diagnose-KO ist ein DPT 16: `writeDpt16Ko()` in OGM-Common ruft bei 15 und mehr Zeichen `fatalError()` auf, `diagnoseInfo()` muss deshalb bei höchstens 14 Zeichen bleiben.

## Regeln für Weiterentwicklung

1. Neue OBIS-Codes bekommen einen eigenen Zweig in `SMLChannel::processDataPoint()` — inkl. `mqttAppend()`, damit KO und MQTT-Snapshot nicht auseinanderlaufen.
2. `SMLModule` verwaltet nur das Channel-Array und den aggregierten Gesamtstatus — keine zählerspezifische Logik dort.
3. Änderungen an der Statuswort-Bit-Tabelle in der Doku klar als unverifizierte Sekundärinformation kennzeichnen, nicht als gesicherte Spezifikation.

## Dokumentation und Hilfe

- Dokumentation liegt in `doc/Applikationsbeschreibung-SML.md`
- Jede sichtbare `ParameterRefRef` bekommt einen `HelpContext` mit Prefix `SML-` (Ausnahme: `Empty` für Parameter ohne eigene Hilfeseite)
- Jede verwendete `HelpContext`-Id muss in der Applikationsbeschreibung als `<!-- DOC HelpContext="SML-..." -->` dokumentiert sein
- Baggages werden über VS Code Task "OpenKNXproducer Documentation" erzeugt (`.vscode/tasks.json`)
- Deutsche Texte mit echten Umlauten (ä, ö, ü, ß) schreiben

## Referenzen

- [README.md](README.md) — Funktionsüberblick, Status-LEDs, Diagnose, Konsolenbefehle
- [doc/Applikationsbeschreibung-SML.md](doc/Applikationsbeschreibung-SML.md) — Statuswort-Details, Konfigtransfer-Beispiele
- [CHANGELOG.md](CHANGELOG.md) — Versionshistorie
