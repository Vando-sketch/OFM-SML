// Parser für OBIS-ASCII-Telegramme (IEC 62056-21 Mode D / "Logarex"-Stil).
// Kein Arduino, keine Exceptions, kein Heap: läuft unverändert auf dem Host (Unit-Tests) und auf dem Zielsystem.
#include "ObisAsciiParser.h"

#include <cstring>

namespace ObisAscii
{
    namespace
    {
        inline bool isDigit(char c)
        {
            return c >= '0' && c <= '9';
        }

        inline bool isLetter(char c)
        {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        }

        inline char toLower(char c)
        {
            return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
        }

        // Vergleicht eine NUL-terminierte Einheit ohne Beachtung der Groß-/Kleinschreibung mit einem
        // kleingeschriebenen Muster.
        bool equalsIgnoreCase(const char *s, const char *lowerPattern)
        {
            while (*lowerPattern != '\0')
            {
                if (toLower(*s) != *lowerPattern)
                    return false;
                s++;
                lowerPattern++;
            }
            return *s == '\0';
        }

        // Liest eine OBIS-Gruppe (mindestens eine Ziffer, Wert 0..255) ab Position i.
        bool readGroup(const char *s, size_t len, size_t &i, uint8_t &value)
        {
            if (i >= len || !isDigit(s[i]))
                return false;
            unsigned v = 0;
            while (i < len && isDigit(s[i]))
            {
                v = v * 10u + (unsigned)(s[i] - '0');
                if (v > 255u)
                    return false;
                i++;
            }
            value = (uint8_t)v;
            return true;
        }

        // Erwartet genau das Zeichen `expected` an Position i und rückt darüber hinaus.
        inline bool expect(const char *s, size_t len, size_t &i, char expected)
        {
            if (i >= len || s[i] != expected)
                return false;
            i++;
            return true;
        }

        // Prüft die Form [+-]?Ziffern(.Ziffern)? und berechnet den Wert (ohne strtod: ohne Locale, klein).
        bool parseNumber(const char *s, size_t len, double &result)
        {
            size_t i = 0;
            bool negative = false;
            if (i < len && (s[i] == '+' || s[i] == '-'))
            {
                negative = (s[i] == '-');
                i++;
            }
            size_t intDigits = 0;
            double mantissa = 0;
            while (i < len && isDigit(s[i]))
            {
                mantissa = mantissa * 10.0 + (double)(s[i] - '0');
                intDigits++;
                i++;
            }
            if (intDigits == 0)
                return false;
            double scale = 1;
            if (i < len && s[i] == '.')
            {
                i++;
                size_t fracDigits = 0;
                while (i < len && isDigit(s[i]))
                {
                    mantissa = mantissa * 10.0 + (double)(s[i] - '0');
                    scale *= 10.0;
                    fracDigits++;
                    i++;
                }
                if (fracDigits == 0)
                    return false;
            }
            if (i != len)
                return false;
            result = negative ? -(mantissa / scale) : (mantissa / scale);
            return true;
        }
    } // namespace

    LineKind parseLine(const char *line, size_t len, Entry &out)
    {
        if (len == 0)
            return LineKind::Empty;
        if (line[0] == '/')
            return LineKind::Ident;
        if (len == 1 && line[0] == '!')
            return LineKind::End;
        if (len > MaxLineLen)
            return LineKind::Invalid;

        // In eine lokale Struktur parsen: `out` bleibt bei Invalid unberührt.
        Entry tmp;
        tmp.a = 1;
        tmp.b = 0;
        tmp.c = 0;
        tmp.d = 0;
        tmp.e = 0;
        tmp.f = 255;
        tmp.letter = '\0';
        tmp.value[0] = '\0';
        tmp.unit[0] = '\0';
        tmp.numeric = false;
        tmp.number = 0;

        // Gruppen A-B:C.D.E*F bzw. Kurzform C.D.E*F; Gruppe C darf ein Buchstabe sein.
        size_t i = 0;
        if (isDigit(line[0]))
        {
            uint8_t first = 0;
            if (!readGroup(line, len, i, first))
                return LineKind::Invalid;
            if (i < len && line[i] == '-')
            {
                tmp.a = first;
                i++;
                if (!readGroup(line, len, i, tmp.b) || !expect(line, len, i, ':'))
                    return LineKind::Invalid;
                if (i < len && isLetter(line[i]))
                {
                    tmp.letter = line[i];
                    i++;
                }
                else if (!readGroup(line, len, i, tmp.c))
                    return LineKind::Invalid;
            }
            else
                tmp.c = first;
        }
        else if (isLetter(line[0]))
        {
            tmp.letter = line[0];
            i = 1;
        }
        else
            return LineKind::Invalid;

        if (!expect(line, len, i, '.') || !readGroup(line, len, i, tmp.d) || !expect(line, len, i, '.') || !readGroup(line, len, i, tmp.e))
            return LineKind::Invalid;
        if (i < len && line[i] == '*')
        {
            i++;
            if (!readGroup(line, len, i, tmp.f))
                return LineKind::Invalid;
        }
        if (!expect(line, len, i, '('))
            return LineKind::Invalid;

        // Inhalt der ersten Klammer: [i, close). Text hinter der ersten ')' wird ignoriert.
        size_t close = i;
        while (close < len && line[close] != ')')
            close++;
        if (close >= len)
            return LineKind::Invalid;

        // Einheit steht hinter dem LETZTEN '*'.
        size_t valueEnd = close;
        size_t unitStart = close;
        for (size_t k = close; k > i; k--)
        {
            if (line[k - 1] == '*')
            {
                valueEnd = k - 1;
                unitStart = k;
                break;
            }
        }
        const size_t valueLen = valueEnd - i;
        const size_t unitLen = close - unitStart;
        if (valueLen > MaxValueLen || unitLen > MaxUnitLen)
            return LineKind::Invalid;

        memcpy(tmp.value, line + i, valueLen);
        tmp.value[valueLen] = '\0';
        memcpy(tmp.unit, line + unitStart, unitLen);
        tmp.unit[unitLen] = '\0';
        tmp.numeric = parseNumber(tmp.value, valueLen, tmp.number);
        if (!tmp.numeric)
            tmp.number = 0;

        out = tmp;
        return LineKind::Data;
    }

    Summary parseTelegram(const char *text, size_t len, LineCallback cb, void *ctx)
    {
        Summary s;
        s.ident[0] = '\0';
        s.lines = 0;
        s.entries = 0;
        s.dropped = 0;
        bool haveIdent = false;

        size_t pos = 0;
        while (pos < len)
        {
            // Zeile bis '\n' (oder Textende); ein abschließendes '\r' gehört nicht zur Zeile.
            const char *line = text + pos;
            size_t end = pos;
            while (end < len && text[end] != '\n')
                end++;
            size_t lineLen = end - pos;
            pos = end + 1;
            if (lineLen > 0 && line[lineLen - 1] == '\r')
                lineLen--;

            Entry entry;
            const LineKind kind = parseLine(line, lineLen, entry);
            if (kind == LineKind::Empty)
                continue;

            if (s.lines < 0xFFFF)
                s.lines++;
            switch (kind)
            {
                case LineKind::Ident:
                    // Die erste Ident-Zeile gilt; ohne führendes '/', auf MaxLineLen gekürzt.
                    if (!haveIdent)
                    {
                        haveIdent = true;
                        size_t n = lineLen - 1;
                        if (n > MaxLineLen)
                            n = MaxLineLen;
                        memcpy(s.ident, line + 1, n);
                        s.ident[n] = '\0';
                    }
                    break;
                case LineKind::Data:
                    if (s.entries < 0xFFFF)
                        s.entries++;
                    break;
                case LineKind::Invalid:
                    if (s.dropped < 0xFFFF)
                        s.dropped++;
                    break;
                default:
                    break;
            }
            if (cb != nullptr)
                cb(ctx, line, lineLen, kind, kind == LineKind::Data ? &entry : nullptr);
        }
        return s;
    }

    double normalizeUnit(double value, const char *unit, char *outUnit, size_t outUnitSize)
    {
        if (unit == nullptr)
            unit = "";
        const char *result = unit;
        if (equalsIgnoreCase(unit, "kwh"))
        {
            value *= 1000.0;
            result = "Wh";
        }
        else if (equalsIgnoreCase(unit, "kw"))
        {
            value *= 1000.0;
            result = "W";
        }

        if (outUnit != nullptr && outUnitSize > 0)
        {
            size_t n = 0;
            while (n < outUnitSize - 1 && result[n] != '\0')
            {
                outUnit[n] = result[n];
                n++;
            }
            outUnit[n] = '\0';
        }
        return value;
    }

    bool isCurrentValue(const Entry &entry)
    {
        return entry.f == 255;
    }

    size_t trimMeterId(const char *in, size_t len, char *out, size_t outSize)
    {
        if (out == nullptr || outSize == 0)
            return 0;
        if (in == nullptr)
            len = 0;

        // Führende Nullen entfernen, solange mehr als 14 Zeichen übrig sind (DPT 16.001).
        size_t start = 0;
        while (len - start > 14 && in[start] == '0')
            start++;

        size_t n = len - start;
        if (n > outSize - 1)
            n = outSize - 1;
        if (n > 0)
            memcpy(out, in + start, n);
        out[n] = '\0';
        return n;
    }
} // namespace ObisAscii

ObisAsciiParser::ObisAsciiParser(char *storage, size_t capacity)
    : _storage(storage), _capacity(capacity), _length(0), _state(State::Idle), _stxSeen(false), _bcc(0),
      _lastDrop(DropReason::None), _bccChecked(false), _droppedLength(0), _rawByte(0), _droppedByte(0)
{
}

ObisAsciiParser::Result ObisAsciiParser::feed(uint8_t byte)
{
    // 7E1-Daten werden als 8N1 gelesen: Paritätsbit ausblenden.
    const char c = (char)(byte & 0x7F);
    _rawByte = byte;
    switch (_state)
    {
        case State::Idle:
            // Nur '/' startet ein Telegramm; alles andere (auch CR/LF nach '!') wird ignoriert.
            // data()/length() des letzten Complete bleiben dabei erhalten.
            if (c == '/' && !beginTelegram())
                return drop(DropReason::Overflow);
            return Result::None;
        case State::Body:
            return feedBody(c);
        case State::Tail:
            return feedTail(c);
        case State::Bcc:
            return feedBcc(c);
    }
    return Result::None;
}

ObisAsciiParser::Result ObisAsciiParser::feedBody(char c)
{
    // STX wird nicht gespeichert, es startet die BCC-Summe.
    if (c == 0x02)
    {
        _stxSeen = true;
        _bcc = 0;
        return Result::None;
    }

    const bool lineStart = _length > 0 && _storage[_length - 1] == '\n';
    if (c == '/' && lineStart)
        return resync();

    // Ein OBIS-ASCII-Telegramm besteht nur aus druckbaren Zeichen und Zeilenenden
    if ((c < 0x20 && c != '\r' && c != '\n' && c != '\t') || c == 0x7F)
    {
        _droppedByte = _rawByte;
        return drop(DropReason::Garbage);
    }

    if (_length >= _capacity)
        return drop(DropReason::Overflow);
    _storage[_length++] = c;
    if (_stxSeen)
        _bcc ^= (uint8_t)c;

    if (c == '!' && lineStart)
    {
        if (!_stxSeen)
            return complete(false);
        _state = State::Tail;
    }
    return Result::None;
}

ObisAsciiParser::Result ObisAsciiParser::feedTail(char c)
{
    // Zwischen '!' und BCC nur CR, LF und ETX; alles zählt zur BCC-Summe.
    if (c == '\r' || c == '\n')
    {
        _bcc ^= (uint8_t)c;
        return Result::None;
    }
    if (c == 0x03)
    {
        _bcc ^= (uint8_t)c;
        _state = State::Bcc;
        return Result::None;
    }
    if (c == '/')
        return resync();
    return drop(DropReason::Framing);
}

ObisAsciiParser::Result ObisAsciiParser::feedBcc(char c)
{
    if ((uint8_t)c == _bcc)
        return complete(true);
    return drop(DropReason::BccMismatch);
}

bool ObisAsciiParser::beginTelegram()
{
    _stxSeen = false;
    _bcc = 0;
    if (_capacity == 0)
    {
        _length = 0;
        _state = State::Idle;
        return false;
    }
    _storage[0] = '/';
    _length = 1;
    _state = State::Body;
    return true;
}

ObisAsciiParser::Result ObisAsciiParser::drop(DropReason reason)
{
    _lastDrop = reason;
    _state = State::Idle;
    _droppedLength = _length;
    _length = 0;
    return Result::Dropped;
}

ObisAsciiParser::Result ObisAsciiParser::resync()
{
    _lastDrop = DropReason::Resync;
    beginTelegram();
    return Result::Dropped;
}

ObisAsciiParser::Result ObisAsciiParser::complete(bool bccChecked)
{
    _bccChecked = bccChecked;
    _state = State::Idle;
    return Result::Complete;
}

const char *ObisAsciiParser::data() const
{
    return _storage;
}

size_t ObisAsciiParser::length() const
{
    return _length;
}

ObisAsciiParser::DropReason ObisAsciiParser::lastDropReason() const
{
    return _lastDrop;
}

size_t ObisAsciiParser::droppedLength() const
{
    return _droppedLength;
}

uint8_t ObisAsciiParser::droppedByte() const
{
    return _droppedByte;
}

bool ObisAsciiParser::bccChecked() const
{
    return _bccChecked;
}

void ObisAsciiParser::reset()
{
    _state = State::Idle;
    _length = 0;
    _stxSeen = false;
    _bcc = 0;
    _lastDrop = DropReason::None;
    _bccChecked = false;
    _droppedLength = 0;
    _droppedByte = 0;
}
