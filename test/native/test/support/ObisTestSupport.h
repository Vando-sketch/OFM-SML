#pragma once
// Gemeinsame Hilfsfunktionen für die OBIS-ASCII-Host-Tests (nur Header, keine eigene Test-Suite).
#include "ObisAsciiParser.h"
#include <unity.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace TestSupport
{
    typedef ObisAsciiParser::Result Res;
    typedef ObisAsciiParser::DropReason Drop;

    // Parser samt eigenem Speicher (Kapazität frei wählbar).
    struct Rig
    {
        std::vector<char> storage;
        ObisAsciiParser parser;
        explicit Rig(size_t capacity = 1024) : storage(capacity), parser(storage.data(), capacity) {}
    };

    // Ergebnis eines Fütterungslaufs: Anzahl und Position (Byteindex) der Ergebnisse.
    struct FeedStats
    {
        int none = 0;
        int complete = 0;
        int dropped = 0;
        long completeAt = -1;
        long droppedAt = -1;
        Res last = Res::None;
    };

    // Füttert alle Bytes von s einzeln in den Parser. Mehrere Aufrufe auf demselben FeedStats addieren sich,
    // die Positionen zählen ab Beginn des jeweiligen Strings.
    inline FeedStats feedAll(ObisAsciiParser &p, const std::string &s)
    {
        FeedStats st;
        for (size_t i = 0; i < s.size(); i++)
        {
            Res r = p.feed((uint8_t)s[i]);
            st.last = r;
            if (r == Res::Complete)
            {
                st.complete++;
                st.completeAt = (long)i;
            }
            else if (r == Res::Dropped)
            {
                st.dropped++;
                st.droppedAt = (long)i;
            }
            else
                st.none++;
        }
        return st;
    }

    // Inhalt von data()/length() als String.
    inline std::string dataOf(const ObisAsciiParser &p)
    {
        return std::string(p.data(), p.length());
    }

    // 7E1-Kodierung: Bit 7 setzen, wenn das 7-Bit-Zeichen eine ungerade Anzahl Einsen hat (gerade Parität).
    inline std::string encode7E1(const std::string &s)
    {
        std::string out;
        for (size_t i = 0; i < s.size(); i++)
        {
            uint8_t c = (uint8_t)s[i] & 0x7F;
            int ones = 0;
            for (int b = 0; b < 7; b++)
                ones += (c >> b) & 1;
            if (ones & 1)
                c |= 0x80;
            out.push_back((char)c);
        }
        return out;
    }

    // BCC: XOR aller (maskierten) Bytes.
    inline uint8_t bccOf(const std::string &s)
    {
        uint8_t x = 0;
        for (size_t i = 0; i < s.size(); i++)
            x ^= ((uint8_t)s[i] & 0x7F);
        return x;
    }

    // Zeilen mit Trenner verbinden (ohne Trenner am Ende).
    inline std::string join(const std::vector<std::string> &lines, const std::string &eol)
    {
        std::string out;
        for (size_t i = 0; i < lines.size(); i++)
        {
            if (i > 0)
                out += eol;
            out += lines[i];
        }
        return out;
    }

    // Beispieltelegramm (Logarex-Stil), 6 Zeilen: Ident, 4 Datenzeilen, "!".
    inline std::vector<std::string> sampleLines()
    {
        std::vector<std::string> l;
        l.push_back("/LOG5LK13BE803039");
        l.push_back("1-0:96.1.0*255(001LOG0067059477)");
        l.push_back("1-0:1.8.0*255(021915.9796*kWh)");
        l.push_back("1-0:16.7.0*255(000696*W)");
        l.push_back("1-0:0.2.0*255(ver.03,432F,20170504)");
        l.push_back("!");
        return l;
    }

    // Beispieltelegramm von '/' bis '!' (ohne abschließendes Zeilenende).
    inline std::string sample(const std::string &eol = "\r\n")
    {
        return join(sampleLines(), eol);
    }

    // Telegramm mit STX/ETX/BCC. head = Zeilen vor STX (Ident), body = Zeilen nach STX (ohne "!"),
    // tail = Text zwischen '!' und ETX (normal "\r\n"). Rückgabe: Rohstrom inkl. BCC-Byte.
    // expectedData (optional): Telegramm von '/' bis '!' wie der Parser es liefern muss.
    inline std::string bccStream(const std::string &head, const std::string &body, const std::string &tail,
                                 std::string *expectedData = nullptr, int bccXor = 0)
    {
        std::string afterStx = body + "!" + tail + std::string(1, (char)0x03);
        std::string raw = head + std::string(1, (char)0x02) + afterStx;
        raw.push_back((char)(bccOf(afterStx) ^ bccXor));
        if (expectedData != nullptr)
            *expectedData = head + body + "!";
        return raw;
    }

    // Bequeme Assert-Helfer.
    inline void assertResult(Res expected, Res actual)
    {
        TEST_ASSERT_EQUAL_INT((int)expected, (int)actual);
    }
    inline void assertDrop(Drop expected, Drop actual)
    {
        TEST_ASSERT_EQUAL_INT((int)expected, (int)actual);
    }
    inline void assertKind(ObisAscii::LineKind expected, ObisAscii::LineKind actual)
    {
        TEST_ASSERT_EQUAL_INT((int)expected, (int)actual);
    }
    inline void assertStr(const std::string &expected, const std::string &actual)
    {
        TEST_ASSERT_EQUAL_STRING(expected.c_str(), actual.c_str());
    }
    inline void assertNear(double expected, double actual)
    {
        TEST_ASSERT_TRUE_MESSAGE(std::fabs(expected - actual) < 1e-6, "double value differs");
    }

    // Entry vollständig prüfen.
    inline void assertEntry(const ObisAscii::Entry &e, int a, int b, int c, int d, int ee, int f, char letter,
                            const char *value, const char *unit, bool numeric, double number)
    {
        TEST_ASSERT_EQUAL_INT(a, e.a);
        TEST_ASSERT_EQUAL_INT(b, e.b);
        TEST_ASSERT_EQUAL_INT(c, e.c);
        TEST_ASSERT_EQUAL_INT(d, e.d);
        TEST_ASSERT_EQUAL_INT(ee, e.e);
        TEST_ASSERT_EQUAL_INT(f, e.f);
        TEST_ASSERT_EQUAL_INT((int)letter, (int)e.letter);
        TEST_ASSERT_EQUAL_STRING(value, e.value);
        TEST_ASSERT_EQUAL_STRING(unit, e.unit);
        TEST_ASSERT_EQUAL_INT(numeric ? 1 : 0, e.numeric ? 1 : 0);
        assertNear(number, e.number);
    }

    // Zeile parsen (String ohne CR/LF).
    inline ObisAscii::LineKind parse(const std::string &line, ObisAscii::Entry &out)
    {
        return ObisAscii::parseLine(line.c_str(), line.size(), out);
    }

    // Sammelt alles, was parseTelegram() per Callback meldet.
    struct Collector
    {
        std::vector<std::string> lines;
        std::vector<ObisAscii::LineKind> kinds;
        std::vector<ObisAscii::Entry> entries; // je Callback ein Eintrag (leer bei kind != Data)
        std::vector<bool> hadEntry;

        static void callback(void *ctx, const char *line, size_t len, ObisAscii::LineKind kind, const ObisAscii::Entry *entry)
        {
            Collector *c = static_cast<Collector *>(ctx);
            c->lines.push_back(std::string(line, len));
            c->kinds.push_back(kind);
            ObisAscii::Entry e;
            memset(&e, 0, sizeof(e));
            if (entry != nullptr)
                e = *entry;
            c->entries.push_back(e);
            c->hadEntry.push_back(entry != nullptr);
        }
    };

    inline ObisAscii::Summary parseWith(const std::string &text, Collector &col)
    {
        return ObisAscii::parseTelegram(text.c_str(), text.size(), &Collector::callback, &col);
    }
} // namespace TestSupport
