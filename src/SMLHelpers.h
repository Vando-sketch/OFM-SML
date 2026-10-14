#pragma once
// Reine Hilfsfunktionen für die Schnittstellenkonfiguration und den Empfangsstatus.
// Hardwareunabhängig (kein Arduino), damit sie auf dem Host getestet werden können.
#include <cstddef>
#include <cstdint>

namespace SMLHelpers
{
    // Serielles Datenformat: 8 Datenbits ohne Parität bzw. 7 Datenbits mit gerader Parität.
    enum class SerialFormat : uint8_t
    {
        F8N1,
        F7E1
    };

    // ETS-Parameter Baudrate (0=9600, 1=300, 2=1200, 3=2400, 4=4800, 5=19200); unbekannt -> 9600.
    uint32_t baudFromParam(uint8_t p);

    // ETS-Parameter Datenformat (0=automatisch, 1=8N1, 2=7E1). Automatisch (und unbekannte Werte)
    // wählt 7E1 für OBIS-ASCII (protocol 1), sonst 8N1.
    SerialFormat formatFromParams(uint8_t protocol, uint8_t dataFormat);

    const char *formatName(SerialFormat fmt);
    const char *protocolName(uint8_t protocol);

    // Empfangs-Timeout in ms. 0 (z. B. nicht initialisierter Parameter) ergibt 15000 (ETS-Standardwert).
    uint32_t timeoutMsFromParam(uint8_t seconds);

    // true, wenn das letzte gültige Telegramm höchstens timeoutMs her ist (überlaufsicher).
    // lastValidTelegram == 0 bedeutet "noch nie empfangen".
    bool isReceiving(uint32_t now, uint32_t lastValidTelegram, uint32_t timeoutMs);

    // true, wenn der Puffer mit einer OBIS-ASCII-Kennungszeile beginnt: '/' gefolgt von
    // mindestens sieben druckbaren Zeichen (Bit 7 wird ausgeblendet, CR/LF sind erlaubt).
    bool looksLikeObisAscii(const uint8_t *buf, size_t len);

    // Schreibt z. B. "ASCII 9600 7E1" bzw. "SML 9600 8N1". Immer NUL-terminiert, gekürzt auf
    // outSize-1. Rückgabe: Anzahl geschriebener Zeichen ohne NUL.
    size_t formatSerialInfo(char *out, size_t outSize, uint8_t protocol, uint32_t baud, SerialFormat fmt);

    // Hex- und ASCII-Dump für das Debug-Log, z. B. "2F 4C 00 |/L.|". Im ASCII-Teil wird Bit 7
    // ausgeblendet (7E1 als 8N1 gelesen), nicht druckbare Zeichen erscheinen als '.'. Passen nicht alle
    // Bytes in out, werden nur so viele ganze Bytes ausgegeben, wie Platz haben. Immer NUL-terminiert.
    // Rückgabe: Anzahl geschriebener Zeichen ohne NUL.
    size_t formatDump(char *out, size_t outSize, const uint8_t *buf, size_t len);

    // Erkennt die SML-Startsequenz 1B1B1B1B 01010101 in einem Bytestrom. feed() liefert true
    // beim letzten Byte der Sequenz. Für den Hinweis "Zähler sendet SML" im OBIS-ASCII-Betrieb.
    struct SmlStartDetector
    {
        uint8_t escapes = 0;
        uint8_t ones = 0;
        bool feed(uint8_t byte);
    };
} // namespace SMLHelpers
