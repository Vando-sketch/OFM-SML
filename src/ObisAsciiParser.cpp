// STUB – wird im nächsten Schritt implementiert
#include "ObisAsciiParser.h"

#include <cstring>

namespace ObisAscii
{
    LineKind parseLine(const char *, size_t, Entry &)
    {
        return LineKind::Invalid;
    }

    Summary parseTelegram(const char *, size_t, LineCallback, void *)
    {
        Summary s;
        memset(&s, 0, sizeof(s));
        return s;
    }

    double normalizeUnit(double, const char *, char *outUnit, size_t outUnitSize)
    {
        if (outUnit != nullptr && outUnitSize > 0)
            outUnit[0] = '\0';
        return 0;
    }

    size_t trimMeterId(const char *, size_t, char *out, size_t outSize)
    {
        if (out != nullptr && outSize > 0)
            out[0] = '\0';
        return 0;
    }
} // namespace ObisAscii

ObisAsciiParser::ObisAsciiParser(char *storage, size_t capacity)
    : _storage(storage), _capacity(capacity), _length(0), _state(State::Idle), _stxSeen(false), _bcc(0),
      _lastDrop(DropReason::None), _bccChecked(false)
{
}

ObisAsciiParser::Result ObisAsciiParser::feed(uint8_t)
{
    return Result::None;
}

const char *ObisAsciiParser::data() const
{
    return _storage;
}

size_t ObisAsciiParser::length() const
{
    return 0;
}

ObisAsciiParser::DropReason ObisAsciiParser::lastDropReason() const
{
    return DropReason::None;
}

bool ObisAsciiParser::bccChecked() const
{
    return false;
}

void ObisAsciiParser::reset()
{
}
