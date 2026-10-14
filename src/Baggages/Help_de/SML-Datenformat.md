### Datenformat

Datenbits, Parität und Stoppbits der Schnittstelle. **Automatisch** wählt passend zum Protokoll 8N1 (SML) bzw. 7E1 (OBIS-ASCII) und ist fast immer richtig. Nur wenn ein Zähler davon abweicht, gibst du das Format hier fest vor.

Steht ein ASCII-Zähler, der mit 7E1 sendet, auf 8N1, werden die Daten trotzdem korrekt gelesen, weil das Paritätsbit beim Auswerten ausgeblendet wird.
