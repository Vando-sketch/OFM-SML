# **Applikationsbeschreibung SML**

<!-- DOC HelpContext="Dokumentation" -->

<!-- DOCCONTENT
Eine vollständige Applikationsbeschreibung ist unter folgendem Link verfügbar: https://github.com/openknx/OFM-SML/blob/v1/doc/Applikationsbeschreibung-SML.md
DOCCONTENT -->

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
<!-- DOCEND -->

## **Allgemein**

Auf dieser Seite sieht man die in der Applikation verwendete Modulversion.

## **MQTT**

Ist im Netzwerkmodul MQTT aktiviert, veröffentlicht jeder Kanal automatisch pro empfangenem Telegramm ein JSON-Objekt mit allen darin enthaltenen Messwerten (z. B. `energy_in`, `power`, `current_l1`, `voltage_l1`, `frequency`, `status`) unter dem Topic `openknx/<geräte-prefix>/sml/<identifikationsnummer>`. Liefert der Zähler keine Identifikationsnummer (OBIS 96.1.0), wird stattdessen der Kanalbuchstabe verwendet (`.../sml/a`, `.../sml/b`, …). Es gibt dafür keinen eigenen ETS-Parameter — die Veröffentlichung folgt automatisch der globalen MQTT-Einstellung im Netzwerkmodul. Gesendet wird ohne Retain-Flag, da die Nachricht keinen Zeitstempel enthält und ein beim nächsten Abonnenten-Connect ausgelieferter, zwischengespeicherter alter Wert irreführend wäre.


<!-- DOC -->
### **Zählertyp**

Hier musst du den Zählertyp auswählen, den du verwenden möchtest. Es wird zwischen Einrichtungs- und Zweirichtungszählern unterschieden. Der Einrichtungszähler ist der klassische Zähler, während der Zweirichtungszähler bei Photovoltaikanlagen verwendet wird, da er die erzeugte Energie erfassen kann. Um welchen Typ es sich handelt, kann am Zähler selbst abgelesen werden. Dieser zeigt bei einem Zweirichtungszähler Stände für 1.8.0 und 2.8.0 im Display an.

Zusätzlich wird zwischen Eintarifzählern (ET) und Zweitarifzählern (HT/NT) unterschieden. Ein Zweitarifzähler hat neben dem Gesamtverbrauch auch zwei weitere Zählerstände für den Hoch- und Niedertarif. Dies kommt häufig bei Nachtspeicherheizungen vor. Zähler mit Tarifumschaltung zeigen im Display neben 1.8.0 auch 1.8.1 bzw. 1.8.2 an.

Generell gilt, dass die Auswahl keine Auswirkung auf die Auswertung bzw. die Kommunikation mit dem Zähler hat. Sie dient lediglich zur Vereinfachung der Oberfläche. Werte, die vom Zähler nicht unterstützt werden, bleiben einfach auf 0 stehen.


<!-- DOC -->
### **Beschreibung**

Freitext zur Bezeichnung des Smartmeters, z. B. "Hauptzähler" oder "PV-Erzeugungszähler". Dieser Text erscheint als Name des Kanal-Reiters, in der Kanalauswahl-Tabelle sowie in den Bezeichnungen der zugehörigen Kommunikationsobjekte.


## **Schnittstelle**

Hier legst du fest, wie der Zähler mit dem Gerät spricht. Für die meisten Zähler passen die Voreinstellungen (SML, 9600 Baud, Datenformat automatisch). Jeder Kanal wird getrennt eingestellt.

Ob der Zähler gerade empfangen wird, zeigt das Kommunikationsobjekt **Zähler empfängt** (DPT 1.002). Es sendet 1, sobald ein gültiges Telegramm ankommt, und 0, wenn innerhalb des Empfangs-Timeouts keines mehr kam. Gesendet wird nur bei einer Änderung. Hat ein Kanal am Gerät keine serielle Schnittstelle (z. B. Kanal B und C beim REG1 Base), bleibt es dauerhaft auf 0, und die Konsole meldet das beim Start einmalig.

<!-- DOC HelpContext="Protokoll" -->
### **Protokoll**

Legt fest, in welchem Format der Zähler seine Daten an der optischen Schnittstelle ausgibt.

- **SML** – Smart Message Language, binär. So senden die meisten modernen Messeinrichtungen (mME) in Deutschland.
- **OBIS-ASCII (IEC 62056-21)** – Klartextzeilen wie `1-0:1.8.0*255(021915.9796*kWh)`, die der Zähler ohne Aufforderung zyklisch sendet (Push-Betrieb), z. B. Logarex LK13BE. Eine Abfrage per Anforderungstelegramm (Mode C) wird nicht unterstützt.

Eine automatische Erkennung gibt es nicht. Welches Protokoll dein Zähler spricht, verrät der Konsolenbefehl **sml debug**: Steht der Kanal auf SML und kommen ASCII-Daten an, erscheint einmalig der Hinweis "Sieht nach OBIS-ASCII aus, Protokoll in der ETS umstellen". Umgekehrt meldet ein auf OBIS-ASCII gestellter Kanal einmalig "Zähler sendet SML, Protokoll in der ETS auf SML umstellen", sobald er die SML-Startsequenz sieht.

Bei OBIS-ASCII gibt es kein Statuswort, der Parameter dazu wird ausgeblendet. Die Identifikationsnummer (OBIS 96.1.0) liefern ASCII-Zähler teils mit führenden Nullen (z. B. 001LOG0067059477). Diese werden entfernt, bis die Nummer wie bei SML 14 Zeichen lang ist und in das Kommunikationsobjekt passt.

Wirkleistung, Spannung und Strom senden viele ASCII-Zähler erst nach Eingabe der PIN und dem Freischalten der erweiterten Datenausgabe (beim Logarex "InF on").

<!-- DOC HelpContext="Baudrate" -->
### **Baudrate**

Übertragungsgeschwindigkeit der Schnittstelle. SML-Zähler und die meisten ASCII-Zähler im Push-Betrieb senden mit 9600 Baud, ältere Zähler teils mit 300 oder 2400 Baud. Stimmt die Baudrate nicht, kommen zwar Bytes an, es wird aber nie ein gültiges Telegramm erkannt. Die Konsole meldet das dann einmalig mit "Daten kommen an, aber kein gültiges OBIS-ASCII-Telegramm". Mit **sml debug** zeigt sie zusätzlich den Anfang der verworfenen Daten als Hex-Dump: lesbare Zeilen wie `1-0:1.8.0` bedeuten, die Einstellung passt; Zeichensalat bedeutet, Baudrate oder Datenformat sind falsch.

<!-- DOC HelpContext="Datenformat" -->
### **Datenformat**

Datenbits, Parität und Stoppbits der Schnittstelle. **Automatisch** wählt passend zum Protokoll 8N1 (SML) bzw. 7E1 (OBIS-ASCII) und ist fast immer richtig. Nur wenn ein Zähler davon abweicht, gibst du das Format hier fest vor.

Steht ein ASCII-Zähler, der mit 7E1 sendet, auf 8N1, werden die Daten trotzdem korrekt gelesen, weil das Paritätsbit beim Auswerten ausgeblendet wird.

<!-- DOC HelpContext="EmpfangsTimeout" -->
### **Empfangs-Timeout**

Zeit in Sekunden, nach der ein Kanal als "empfängt nicht" gilt, wenn bis dahin kein gültiges Telegramm angekommen ist. Danach wechselt die Kanal-Status-LED auf rot bzw. aus, und **Zähler empfängt** sendet 0. Der Standardwert von 5 Sekunden passt für Zähler, die etwa jede Sekunde senden. Sendet dein Zähler seltener, erhöhe den Wert, sonst springt der Status zwischen den Telegrammen hin und her. Ist der Timeout kürzer als der tatsächliche Sendeabstand, meldet die Konsole das einmalig mit dem gemessenen Abstand ("Zähler sendet nur alle … s"). Mit **sml debug** steht der Abstand bei jedem Telegramm im Log.

<!-- DOC -->
## **Zählerstände**

Hier werden die Zählerstände (Wirkarbeit) konfiguriert und über KNX bereitgestellt. Je nach oben gewähltem Zählertyp stehen die Zählerstände für den Bezug (OBIS 1.8.0), bei Zweitarifzählern zusätzlich getrennt nach Hoch- und Niedertarif (1.8.1/1.8.2), und bei Zweirichtungszählern zusätzlich für die Einspeisung (2.8.0, ggf. 2.8.1/2.8.2) zur Verfügung. Für jeden Zählerstand kann getrennt festgelegt werden, bei welcher Änderung und/oder in welchem festen Zyklus der aktuelle Wert gesendet wird.

Zusätzlich lässt sich pro Zählerstand eine Wh-genaue Variante aktivieren, die primär zur Weiterverarbeitung mit dem Logikmodul oder dem virtuellen Zähler gedacht ist und nicht zur Übertragung per Gruppenadresse. Deshalb ist das Ü-Flag bei diesen Kommunikationsobjekten standardmäßig deaktiviert; willst du den Wh-Wert doch per Gruppenadresse senden, musst du es manuell wieder aktivieren.

<!-- DOC HelpContext="AenderungUndZyklus" -->
### **Änderung und Zyklus**

Die beiden Schalter "Sende bei Änderung" und "Sende nur alle" schließen sich nicht gegenseitig aus, sondern werden mit ODER verknüpft: Der aktuelle Wert wird gesendet, sobald eine der beiden Bedingungen zutrifft — entweder die eingestellte Änderungsschwelle wurde überschritten, oder das eingestellte Zeitintervall ist abgelaufen. Sind beide Schalter deaktiviert, wird der Wert nie aktiv auf den Bus gesendet, steht aber bei einem Lesevorgang weiterhin aktuell zur Verfügung.

<!-- DOC -->
## **Messwerte**

Neben der eigentlichen Aufgabe, den Stromverbrauch zu messen, senden die Zähler auch verschiedene weitere Messwerte. Dazu gehören neben der üblichen Wirkleistung auch Werte wie Spannung, Strom und Frequenz. Um diese zusätzlichen Messwerte zu erhalten, ist in der Regel die Eingabe einer PIN erforderlich, die ihr bei eurem Messstellenbetreiber anfordern könnt. Der Umfang der verfügbaren Messwerte variiert jedoch je nach Betreiber und Zählermodell. Manche Zähler geben zum Beispiel nur die Wirkleistung aus. Wenn die Schnittstelle keine Werte liefert, bleiben die betroffenen KOs einfach bei 0 stehen.

Die Wirkleistung lässt sich getrennt als Summe und als Einzelwerte je Phase aktivieren, jeweils mit eigenem Sende-Intervall. Der Summenwert ist meist der wichtigere und wird häufiger benötigt, während die drei Phasenwerte seltener gebraucht werden. Durch die Trennung lässt sich der Summenwert zum Beispiel alle 5 Sekunden senden, ohne dass zusätzlich drei weitere Telegramme für die einzelnen Phasen den Bus belasten.

<!-- DOC -->
## **Erweitert**

Hier lassen sich zusätzliche, optionale Informationen des Zählers aktivieren, die für die eigentliche Zählerauswertung nicht notwendig sind.

<!-- DOC HelpContext="Identifikationsnummer" -->
### **Identifikationsnummer**

Aktiviert die Identifikationsnummer des Zählers (Hersteller, Sparte und Seriennummer nach OBIS 96.1.0). Ist die Option eingeschaltet, sendet das zugehörige Kommunikationsobjekt die vom Zähler gemeldete Identifikationsnummer als Text.

<!-- DOC HelpContext="Statuswort" -->
### **Statuswort**

Aktiviert das vom Zähler gemeldete Statuswort. Das Feld ist allerdings optional. Außerdem sind die Bits nicht sauber spezifiziert. Ich habe aber zwei Tabellen für 16 und 32 Bit gefunden. Daher wird der Wert einfach roh ausgegeben, und man kann die Auswertung z.B. mit dem Logikmodul machen.

Zähler mit nur 16 Bit landen in den unteren 16 Bit des Kommunikationsobjekts, der Rest bleibt 0. Gesendet wird nur bei einer Änderung des Wertes.

Das Ü-Flag ist deshalb standardmäßig deaktiviert: einzelne Bits (z.B. die Leerlauf-/Anlauferkennung) können sich je nach Last auch mal sekündlich ändern, und das willst du dir nicht auf den Bus holen. Werte das Statuswort daher am besten geräteintern mit dem Logikmodul aus. Willst du den Rohwert trotzdem wirklich per Gruppenadresse verschicken, musst du das Ü-Flag am Kommunikationsobjekt manuell wieder aktivieren.

Anbei gibt es neben der Bit-Tabelle noch zwei Beispiele, die du mit dem Konfigtransfer einlesen kannst. Beide gehen vom 32-Bit-Status aus, weil die meisten Zähler den nutzen.

**Beispiel 1: Fehlerauswertung** — wertet die Bits 9, 10 und 17 aus und macht daraus ein Fehler-KO.

    OpenKNX,cv1,0xAF05:0xA3/LOG:0x44/10§f~Name=Z%C3%A4hlerstatus%20(Fehler)§f~LogicType=2§f~Logic=2§f~NameInput1=Stromz%C3%A4hler%20Statusmeldung§f~E1ConvertFloat=7§f~E1=1§f~E1Dpt=13§f~E1OtherKO:2=524§f~E1UseOtherKO=1§f~NameInput2=Konstante%3A%20Bits%209%2C10%2C17§f~E2ConvertFloat=5§f~E2=1§f~E2Dpt=13§f~E2LowDpt12Fix=132608§f~NameOutput=Ausgewerteter%20Fehlerstatus§f~OSendOnChange=1§f~OOnAll=8§f~OOnFunction=9§f~OOffAll=0§;OpenKNX

**Beispiel 2: Energieflussrichtung gesamt** — wertet Bit 11 aus und macht daraus ein KO für die Energierichtung (Bezug/Einspeisung).

    OpenKNX,cv1,0xAF05:0xA3/LOG:0x44/1§f~Name=Energierichtung%20gesamt§f~LogicType=2§f~Logic=2§f~NameInput1=Stromz%C3%A4hler%20Statusmeldung§f~E1ConvertFloat=7§f~E1=1§f~E1Dpt=13§f~E1OtherKO:2=524§f~E1UseOtherKO=1§f~NameInput2=Konstante%3A%20Bit%2011§f~E2ConvertFloat=5§f~E2=1§f~E2Dpt=13§f~E2LowDpt12Fix=2048§f~NameOutput=Ausgewertete%20Energierichtung§f~OSendOnChange=1§f~OOnAll=8§f~OOnFunction=9§f~OOffAll=0§;OpenKNX

*Bit-Tabellen*

- **16-Bit-Wort:**
  - Bit 0: Fehler
  - Bit 1: metrologische Synchronisation
  - Bit 2: reserviert, immer 0
  - Bit 3: Zeit-Synchronisation
  - Bit 4: Rücklaufsperre
  - Bit 5: Energierichtung
  - Bit 6: magnetische Manipulation
  - Bit 7: Leerlauf-/Anlauferkennung
  - Bit 8–9: Tarif 1/2 aktiv
  - Bit 10–15: aktuelle Tarifinformation

- **32-Bit-Wort:**
  - Bit 0–1: immer 0
  - Bit 2: immer 1 (Versionsmarker)
  - Bit 3–7: immer 0
  - Bit 8: Leerlauf-/Anlauferkennung
  - Bit 9: magnetische Manipulation
  - Bit 10: Manipulation Klemmendeckel/Plombierstift
  - Bit 11–14: Energierichtung Summe/L1/L2/L3
  - Bit 15: Drehfeld
  - Bit 16: Rücklaufsperre
  - Bit 17: Fehler eichrelevant
  - Bit 18–20: Leiterspannung L1/L2/L3 vorhanden
  - Bit 21–31: reserviert, immer 0

Praktisch ist Bit 2: beim 16-Bit-Wort ist das immer 0, beim 32-Bit-Wort immer 1. Damit siehst du am Rohwert direkt, welches der beiden Formate dein Zähler tatsächlich nutzt.


