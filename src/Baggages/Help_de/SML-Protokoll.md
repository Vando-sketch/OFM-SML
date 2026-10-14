### Protokoll

Legt fest, in welchem Format der Zähler seine Daten an der optischen Schnittstelle ausgibt.

- **SML** – Smart Message Language, binär. So senden die meisten modernen Messeinrichtungen (mME) in Deutschland.
- **OBIS-ASCII (IEC 62056-21)** – Klartextzeilen wie `1-0:1.8.0*255(021915.9796*kWh)`, die der Zähler ohne Aufforderung zyklisch sendet (Push-Betrieb), z. B. Logarex LK13BE. Eine Abfrage per Anforderungstelegramm (Mode C) wird nicht unterstützt.

Eine automatische Erkennung gibt es nicht. Welches Protokoll dein Zähler spricht, verrät der Konsolenbefehl **sml debug**: Steht der Kanal auf SML und kommen ASCII-Daten an, erscheint einmalig der Hinweis "Sieht nach OBIS-ASCII aus, Protokoll in der ETS umstellen".

Bei OBIS-ASCII gibt es kein Statuswort, der Parameter dazu wird ausgeblendet. Die Identifikationsnummer (OBIS 96.1.0) liefern ASCII-Zähler teils mit führenden Nullen (z. B. 001LOG0067059477). Diese werden entfernt, bis die Nummer wie bei SML 14 Zeichen lang ist und in das Kommunikationsobjekt passt.

Wirkleistung, Spannung und Strom senden viele ASCII-Zähler erst nach Eingabe der PIN und dem Freischalten der erweiterten Datenausgabe (beim Logarex "InF on").
