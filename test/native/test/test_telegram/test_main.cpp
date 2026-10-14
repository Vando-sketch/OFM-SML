// Tests für ObisAscii::parseTelegram (Zerlegen eines kompletten Telegramms) und das Zusammenspiel
// mit dem Rahmenparser.
#include "../support/ObisTestSupport.h"

using namespace TestSupport;
using ObisAscii::Entry;
using ObisAscii::LineKind;
using ObisAscii::Summary;

void setUp(void) {}
void tearDown(void) {}

// Prüft Summary und Callback-Ablauf des Beispieltelegramms vollständig.
static void checkSampleParse(const std::string &text)
{
    Collector col;
    Summary s = parseWith(text, col);
    TEST_ASSERT_EQUAL_STRING("LOG5LK13BE803039", s.ident);
    TEST_ASSERT_EQUAL_UINT16(6, s.lines);
    TEST_ASSERT_EQUAL_UINT16(4, s.entries);
    TEST_ASSERT_EQUAL_UINT16(0, s.dropped);

    std::vector<std::string> lines = sampleLines();
    TEST_ASSERT_EQUAL_UINT(6, col.lines.size());
    const LineKind kinds[6] = {LineKind::Ident, LineKind::Data, LineKind::Data, LineKind::Data, LineKind::Data, LineKind::End};
    for (size_t i = 0; i < 6; i++)
    {
        assertStr(lines[i], col.lines[i]); // roh, ohne CR
        assertKind(kinds[i], col.kinds[i]);
        TEST_ASSERT_EQUAL_INT(kinds[i] == LineKind::Data ? 1 : 0, col.hadEntry[i] ? 1 : 0);
    }
    assertEntry(col.entries[1], 1, 0, 96, 1, 0, 255, '\0', "001LOG0067059477", "", false, 0);
    assertEntry(col.entries[2], 1, 0, 1, 8, 0, 255, '\0', "021915.9796", "kWh", true, 21915.9796);
    assertEntry(col.entries[3], 1, 0, 16, 7, 0, 255, '\0', "000696", "W", true, 696);
    assertEntry(col.entries[4], 1, 0, 0, 2, 0, 255, '\0', "ver.03,432F,20170504", "", false, 0);
}

// ---------------------------------------------------------------- Beispieltelegramm

static void test_sample_crlf()
{
    checkSampleParse(sample("\r\n"));
}

static void test_sample_lf_only()
{
    checkSampleParse(sample("\n"));
}

static void test_sample_with_trailing_eol_and_blank_lines()
{
    // Leere Zeilen werden übersprungen: kein Callback, nicht gezählt.
    checkSampleParse(sample("\r\n") + "\r\n\r\n");
    std::vector<std::string> l = sampleLines();
    std::string spaced = l[0] + "\r\n\r\n" + l[1] + "\r\n" + l[2] + "\r\n\r\n\r\n" + l[3] + "\r\n" + l[4] + "\r\n" + l[5];
    checkSampleParse(spaced);
}

static void test_sample_null_callback()
{
    std::string t = sample("\r\n");
    Summary s = ObisAscii::parseTelegram(t.c_str(), t.size(), nullptr, nullptr);
    TEST_ASSERT_EQUAL_STRING("LOG5LK13BE803039", s.ident);
    TEST_ASSERT_EQUAL_UINT16(6, s.lines);
    TEST_ASSERT_EQUAL_UINT16(4, s.entries);
    TEST_ASSERT_EQUAL_UINT16(0, s.dropped);
}

static void test_framed_stream_through_parser_then_parse()
{
    // komplette Kette: Bytes -> Rahmenparser -> parseTelegram
    const std::string streams[] = {sample("\r\n") + "\r\n", sample("\n") + "\n", encode7E1(sample("\r\n") + "\r\n")};
    for (size_t i = 0; i < 3; i++)
    {
        Rig r;
        FeedStats st = feedAll(r.parser, streams[i]);
        TEST_ASSERT_EQUAL_INT(1, st.complete);
        checkSampleParse(dataOf(r.parser));
    }
}

static void test_framed_bcc_stream_then_parse()
{
    std::vector<std::string> l = sampleLines();
    std::string body = l[1] + "\r\n" + l[2] + "\r\n" + l[3] + "\r\n" + l[4] + "\r\n";
    std::string raw = bccStream(l[0] + "\r\n", body, "\r\n");
    Rig r;
    FeedStats st = feedAll(r.parser, encode7E1(raw));
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_TRUE(r.parser.bccChecked());
    checkSampleParse(dataOf(r.parser));
}

// ---------------------------------------------------------------- Fehlerhafte Zeilen

static void test_garbage_lines_in_the_middle()
{
    std::string tel = join({"/LOG5LK13BE803039", "1-0:96.1.0*255(001LOG0067059477)", "#garbage-line#",
                            "1-0:1.8.0*255(021915.9796*kWh", "1-0:16.7.0*255(000696*W)", "!"},
                           "\r\n");
    // über den Rahmenparser: kommt trotzdem komplett an
    Rig r;
    FeedStats st = feedAll(r.parser, tel + "\r\n");
    TEST_ASSERT_EQUAL_INT(1, st.complete);

    Collector col;
    Summary s = parseWith(dataOf(r.parser), col);
    TEST_ASSERT_EQUAL_STRING("LOG5LK13BE803039", s.ident);
    TEST_ASSERT_EQUAL_UINT16(6, s.lines);
    TEST_ASSERT_EQUAL_UINT16(2, s.entries);
    TEST_ASSERT_EQUAL_UINT16(2, s.dropped);

    const LineKind kinds[6] = {LineKind::Ident, LineKind::Data, LineKind::Invalid, LineKind::Invalid, LineKind::Data, LineKind::End};
    TEST_ASSERT_EQUAL_UINT(6, col.kinds.size());
    for (size_t i = 0; i < 6; i++)
    {
        assertKind(kinds[i], col.kinds[i]);
        TEST_ASSERT_EQUAL_INT(kinds[i] == LineKind::Data ? 1 : 0, col.hadEntry[i] ? 1 : 0);
    }
    assertStr("#garbage-line#", col.lines[2]);
    assertStr("1-0:1.8.0*255(021915.9796*kWh", col.lines[3]);
    // gültige Einträge vor UND nach den fehlerhaften Zeilen sind vorhanden
    assertEntry(col.entries[1], 1, 0, 96, 1, 0, 255, '\0', "001LOG0067059477", "", false, 0);
    assertEntry(col.entries[4], 1, 0, 16, 7, 0, 255, '\0', "000696", "W", true, 696);
}

static void test_invalid_line_does_not_discard_rest()
{
    Collector col;
    Summary s = parseWith("/X\r\n#bad#\r\n1.8.0(1*kWh)\r\n!", col);
    TEST_ASSERT_EQUAL_UINT16(4, s.lines);
    TEST_ASSERT_EQUAL_UINT16(1, s.entries);
    TEST_ASSERT_EQUAL_UINT16(1, s.dropped);
}

// ---------------------------------------------------------------- Randfälle

static void test_no_ident()
{
    Collector col;
    Summary s = parseWith("1.8.0(1*kWh)\n", col);
    TEST_ASSERT_EQUAL_STRING("", s.ident);
    TEST_ASSERT_EQUAL_UINT16(1, s.lines);
    TEST_ASSERT_EQUAL_UINT16(1, s.entries);
    TEST_ASSERT_EQUAL_UINT16(0, s.dropped);
}

static void test_empty_text()
{
    Collector col;
    Summary s = ObisAscii::parseTelegram("", 0, &Collector::callback, &col);
    TEST_ASSERT_EQUAL_STRING("", s.ident);
    TEST_ASSERT_EQUAL_UINT16(0, s.lines);
    TEST_ASSERT_EQUAL_UINT16(0, s.entries);
    TEST_ASSERT_EQUAL_UINT16(0, s.dropped);
    TEST_ASSERT_EQUAL_UINT(0, col.lines.size());
}

static void test_only_blank_lines()
{
    Collector col;
    Summary s = parseWith("\r\n\n\r\n", col);
    TEST_ASSERT_EQUAL_UINT16(0, s.lines);
    TEST_ASSERT_EQUAL_UINT(0, col.lines.size());
}

static void test_last_line_without_eol()
{
    Collector col;
    Summary s = parseWith("/X\r\n1.8.0(7*kWh)", col);
    TEST_ASSERT_EQUAL_UINT16(2, s.lines);
    TEST_ASSERT_EQUAL_UINT16(1, s.entries);
    assertStr("1.8.0(7*kWh)", col.lines[1]);
}

static void test_len_parameter_is_honoured()
{
    // Text über `len` hinaus wird nicht gelesen.
    std::string t = "/X\r\n1.8.0(1*kWh)\r\n2.8.0(2*kWh)\r\n";
    Collector col;
    Summary s = ObisAscii::parseTelegram(t.c_str(), strlen("/X\r\n1.8.0(1*kWh)\r\n"), &Collector::callback, &col);
    TEST_ASSERT_EQUAL_UINT16(2, s.lines);
    TEST_ASSERT_EQUAL_UINT16(1, s.entries);
}

// ---------------------------------------------------------------- Feldtelegramm Logarex LK13BE mit "InF on"

// Originaltelegramm aus einem Feldlog (674 Byte, Leerzeile nach der Kennung). Enthält Vorwerte
// 1.8.0*96 … *100 (Verbrauch der letzten Tage/Monate/Jahre), die NICHT als Zählerstand gelten dürfen.
static std::string fieldTelegram()
{
    return "/LOG5LK13BE803039\r\n\r\n"
           "1-0:96.1.0*255(001LOG0065547018)\r\n"
           "1-0:1.8.0*255(007003.0386*kWh)\r\n"
           "1-0:2.8.0*255(000000.0000*kWh)\r\n"
           "1-0:16.7.0*255(000395*W)\r\n"
           "1-0:32.7.0*255(238.6*V)\r\n"
           "1-0:52.7.0*255(238.3*V)\r\n"
           "1-0:72.7.0*255(239.3*V)\r\n"
           "1-0:31.7.0*255(000.71*A)\r\n"
           "1-0:51.7.0*255(000.84*A)\r\n"
           "1-0:71.7.0*255(000.80*A)\r\n"
           "1-0:81.7.1*255(117*deg)\r\n"
           "1-0:81.7.2*255(234*deg)\r\n"
           "1-0:81.7.4*255(053*deg)\r\n"
           "1-0:81.7.15*255(056*deg)\r\n"
           "1-0:81.7.26*255(017*deg)\r\n"
           "1-0:14.7.0*255(49.9*Hz)\r\n"
           "1-0:1.8.0*96(00005.4*kWh)\r\n"
           "1-0:1.8.0*97(00044.5*kWh)\r\n"
           "1-0:1.8.0*98(00438.4*kWh)\r\n"
           "1-0:1.8.0*99(04205.1*kWh)\r\n"
           "1-0:1.8.0*100(07003.0*kWh)\r\n"
           "1-0:0.2.0*255(ver.03,432F,20170504)\r\n"
           "1-0:96.90.2*255(0F66)\r\n"
           "1-0:97.97.0*255(00000000)\r\n"
           "!";
}

static void test_field_telegram_frames_as_in_log()
{
    Rig r(1024);
    std::string tel = fieldTelegram();
    FeedStats st = feedAll(r.parser, encode7E1(tel) + "\r\n");
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_EQUAL_INT(0, st.dropped);
    TEST_ASSERT_EQUAL_UINT(674, r.parser.length()); // "Valid OBIS-ASCII telegram (674 bytes)"

    Collector col;
    Summary s = parseWith(dataOf(r.parser), col);
    TEST_ASSERT_EQUAL_UINT16(26, s.lines); // "26 Zeilen, 24 Werte, 0 verworfen"
    TEST_ASSERT_EQUAL_UINT16(24, s.entries);
    TEST_ASSERT_EQUAL_UINT16(0, s.dropped);
}

static void test_field_telegram_only_current_1_8_0_counts()
{
    Collector col;
    parseWith(fieldTelegram(), col);
    int current = 0, history = 0;
    double value = 0;
    for (size_t i = 0; i < col.entries.size(); i++)
    {
        if (!col.hadEntry[i]) continue;
        const Entry &e = col.entries[i];
        if (e.c != 1 || e.d != 8 || e.e != 0) continue;
        if (ObisAscii::isCurrentValue(e))
        {
            current++;
            value = e.number;
        }
        else
            history++;
    }
    TEST_ASSERT_EQUAL_INT(1, current);
    TEST_ASSERT_EQUAL_INT(5, history);
    assertNear(7003.0386, value);
}

static void test_current_value_rule()
{
    Entry e;
    memset(&e, 0, sizeof(e));
    e.f = 255;
    TEST_ASSERT_TRUE(ObisAscii::isCurrentValue(e)); // *255 und Kurzform ohne F
    e.f = 96;
    TEST_ASSERT_FALSE(ObisAscii::isCurrentValue(e));
    e.f = 0;
    TEST_ASSERT_FALSE(ObisAscii::isCurrentValue(e));
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_sample_crlf);
    RUN_TEST(test_sample_lf_only);
    RUN_TEST(test_sample_with_trailing_eol_and_blank_lines);
    RUN_TEST(test_sample_null_callback);
    RUN_TEST(test_framed_stream_through_parser_then_parse);
    RUN_TEST(test_framed_bcc_stream_then_parse);
    RUN_TEST(test_garbage_lines_in_the_middle);
    RUN_TEST(test_invalid_line_does_not_discard_rest);
    RUN_TEST(test_no_ident);
    RUN_TEST(test_empty_text);
    RUN_TEST(test_only_blank_lines);
    RUN_TEST(test_last_line_without_eol);
    RUN_TEST(test_len_parameter_is_honoured);
    RUN_TEST(test_field_telegram_frames_as_in_log);
    RUN_TEST(test_field_telegram_only_current_1_8_0_counts);
    RUN_TEST(test_current_value_rule);
    return UNITY_END();
}
