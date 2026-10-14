### Baudrate

Übertragungsgeschwindigkeit der Schnittstelle. SML-Zähler und die meisten ASCII-Zähler im Push-Betrieb senden mit 9600 Baud, ältere Zähler teils mit 300 oder 2400 Baud. Stimmt die Baudrate nicht, kommen zwar Bytes an, es wird aber nie ein gültiges Telegramm erkannt. Die Konsole meldet das dann einmalig mit "Daten kommen an, aber kein gültiges OBIS-ASCII-Telegramm". Mit **sml debug** zeigt sie zusätzlich den Anfang der verworfenen Daten als Hex-Dump: lesbare Zeilen wie `1-0:1.8.0` bedeuten, die Einstellung passt; Zeichensalat bedeutet, Baudrate oder Datenformat sind falsch.
