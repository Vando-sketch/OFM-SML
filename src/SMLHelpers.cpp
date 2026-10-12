#include "SMLHelpers.h"

#include <cstdio>
#include <cstring>

namespace SMLHelpers
{
    uint32_t baudFromParam(uint8_t p)
    {
        switch (p)
        {
            case 1: return 300;
            case 2: return 1200;
            case 3: return 2400;
            case 4: return 4800;
            case 5: return 19200;
            case 0:
            default: return 9600;
        }
    }

    SerialFormat formatFromParams(uint8_t protocol, uint8_t dataFormat)
    {
        if (dataFormat == 1) return SerialFormat::F8N1;
        if (dataFormat == 2) return SerialFormat::F7E1;

        // Automatisch (0) bzw. unbekannter Wert: OBIS-ASCII wird üblicherweise 7E1 gesendet
        return protocol == 1 ? SerialFormat::F7E1 : SerialFormat::F8N1;
    }

    const char *formatName(SerialFormat fmt)
    {
        return fmt == SerialFormat::F7E1 ? "7E1" : "8N1";
    }

    const char *protocolName(uint8_t protocol)
    {
        return protocol == 1 ? "ASCII" : "SML";
    }

    uint32_t timeoutMsFromParam(uint8_t seconds)
    {
        // 0 liegt außerhalb des ETS-Bereichs (1..255), z. B. bei nicht initialisiertem Speicher
        if (seconds == 0) return 5000;
        return (uint32_t)seconds * 1000;
    }

    bool isReceiving(uint32_t now, uint32_t lastValidTelegram, uint32_t timeoutMs)
    {
        if (lastValidTelegram == 0) return false;
        return (uint32_t)(now - lastValidTelegram) < timeoutMs;
    }

    bool looksLikeObisAscii(const uint8_t *buf, size_t len)
    {
        if (buf == nullptr || len < 8) return false;
        if ((buf[0] & 0x7F) != '/') return false;

        for (size_t i = 1; i < 8; i++)
        {
            const uint8_t c = buf[i] & 0x7F;
            if (!((c >= 0x20 && c <= 0x7E) || c == '\r' || c == '\n')) return false;
        }
        return true;
    }

    size_t formatSerialInfo(char *out, size_t outSize, uint8_t protocol, uint32_t baud, SerialFormat fmt)
    {
        if (out == nullptr || outSize == 0) return 0;

        const int n = snprintf(out, outSize, "%s %lu %s", protocolName(protocol), (unsigned long)baud, formatName(fmt));
        if (n < 0)
        {
            out[0] = '\0';
            return 0;
        }
        return (size_t)n >= outSize ? outSize - 1 : (size_t)n;
    }
} // namespace SMLHelpers
