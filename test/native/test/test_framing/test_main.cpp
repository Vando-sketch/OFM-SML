// Tests für den byteweisen Rahmenparser ObisAsciiParser (Framing, Resync, Overflow, BCC, reset).
#include "../support/ObisTestSupport.h"

using namespace TestSupport;

void setUp(void) {}
void tearDown(void) {}

static const char STX = 0x02;
static const char ETX = 0x03;

// ---------------------------------------------------------------- Grundzustand / Standardfälle

static void test_initial_state()
{
    Rig r;
    TEST_ASSERT_EQUAL_UINT(0, r.parser.length());
    assertDrop(Drop::None, r.parser.lastDropReason());
    TEST_ASSERT_FALSE(r.parser.bccChecked());
}

static void test_idle_ignores_bytes_until_slash()
{
    Rig r;
    FeedStats st = feedAll(r.parser, "xyz\r\n!\r\n1-0:1.8.0(1)\r\n");
    TEST_ASSERT_EQUAL_INT(0, st.complete);
    TEST_ASSERT_EQUAL_INT(0, st.dropped);
    TEST_ASSERT_EQUAL_UINT(0, r.parser.length());
}

static void test_sample_crlf_byte_by_byte()
{
    Rig r;
    std::string tel = sample("\r\n");
    FeedStats st = feedAll(r.parser, tel + "\r\n");
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_EQUAL_INT(0, st.dropped);
    // Complete genau auf dem '!'
    TEST_ASSERT_EQUAL_INT((long)tel.size() - 1, st.completeAt);
    // alle anderen Bytes liefern None (auch das CRLF nach '!')
    TEST_ASSERT_EQUAL_INT((int)(tel.size() + 2 - 1), st.none);
    assertStr(tel, dataOf(r.parser));
    TEST_ASSERT_EQUAL_UINT(tel.size(), r.parser.length());
    TEST_ASSERT_FALSE(r.parser.bccChecked());
}

static void test_sample_lf_only()
{
    Rig r;
    std::string tel = sample("\n");
    FeedStats st = feedAll(r.parser, tel + "\n");
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_EQUAL_INT(0, st.dropped);
    TEST_ASSERT_EQUAL_INT((long)tel.size() - 1, st.completeAt);
    assertStr(tel, dataOf(r.parser));
    TEST_ASSERT_FALSE(r.parser.bccChecked());
}

static void test_sample_7e1_same_data_as_plain()
{
    Rig plain;
    Rig coded;
    std::string tel = sample("\r\n");
    FeedStats a = feedAll(plain.parser, tel);
    FeedStats b = feedAll(coded.parser, encode7E1(tel));
    TEST_ASSERT_EQUAL_INT(1, a.complete);
    TEST_ASSERT_EQUAL_INT(1, b.complete);
    TEST_ASSERT_EQUAL_INT(a.completeAt, b.completeAt);
    TEST_ASSERT_EQUAL_UINT(plain.parser.length(), coded.parser.length());
    assertStr(dataOf(plain.parser), dataOf(coded.parser));
    assertStr(tel, dataOf(coded.parser));
}

static void test_encode7e1_helper_really_sets_bit7()
{
    // Sicherheitsnetz für den Test selbst: 7E1 muss sich vom Klartext unterscheiden.
    std::string tel = sample("\r\n");
    TEST_ASSERT_TRUE(encode7E1(tel) != tel);
}

static void test_data_stays_valid_after_complete_until_next_feed()
{
    Rig r;
    std::string tel = sample("\r\n");
    feedAll(r.parser, tel);
    assertStr(tel, dataOf(r.parser));
    // erneutes Lesen ändert nichts
    TEST_ASSERT_EQUAL_UINT(tel.size(), r.parser.length());
    assertStr(tel, dataOf(r.parser));
}

static void test_two_telegrams_back_to_back()
{
    Rig r;
    std::string t1 = "/A\r\n1-0:1.8.0(1*kWh)\r\n!";
    std::string t2 = "/B\r\n1-0:16.7.0(5*W)\r\n!";
    FeedStats s1 = feedAll(r.parser, t1 + "\r\n");
    TEST_ASSERT_EQUAL_INT(1, s1.complete);
    assertStr(t1, dataOf(r.parser));
    FeedStats s2 = feedAll(r.parser, t2 + "\r\n");
    TEST_ASSERT_EQUAL_INT(1, s2.complete);
    TEST_ASSERT_EQUAL_INT(0, s2.dropped);
    assertStr(t2, dataOf(r.parser));
}

static void test_bang_and_slash_only_matter_at_line_start()
{
    // Annahme: '!' bzw. '/' mitten in einer Zeile sind gewöhnliche Datenbytes.
    Rig r;
    std::string tel = "/X\r\n1-0:1.8.0(a/b)\r\nfoo!bar\r\n!";
    FeedStats st = feedAll(r.parser, tel);
    TEST_ASSERT_EQUAL_INT(0, st.dropped);
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_EQUAL_INT((long)tel.size() - 1, st.completeAt);
    assertStr(tel, dataOf(r.parser));
}

// ---------------------------------------------------------------- Abbruch / Resync

static void test_truncated_never_completes()
{
    std::string tel = sample("\r\n");
    // jeder echte Präfix (ohne das abschließende '!') darf nie Complete liefern
    for (size_t cut = 1; cut < tel.size(); cut++)
    {
        Rig r;
        FeedStats st = feedAll(r.parser, tel.substr(0, cut));
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, st.complete, "Präfix darf nicht komplett sein");
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, st.dropped, "Präfix darf nicht verworfen werden");
    }
}

static void test_truncated_mid_line_then_silence()
{
    Rig r;
    FeedStats st = feedAll(r.parser, "/LOG5LK13BE803039\r\n1-0:96.1.0*255(001LOG00");
    TEST_ASSERT_EQUAL_INT(0, st.complete);
    TEST_ASSERT_EQUAL_INT(0, st.dropped);
}

static void test_resync_on_slash_at_line_start()
{
    Rig r;
    std::string first = "/A\r\n1-0:1.8.0(1*kWh)\r\n"; // '!' fehlt
    std::string second = "/B\r\n1-0:16.7.0(5*W)\r\n!";
    FeedStats s1 = feedAll(r.parser, first);
    TEST_ASSERT_EQUAL_INT(0, s1.complete);
    TEST_ASSERT_EQUAL_INT(0, s1.dropped);

    // das '/' am Zeilenanfang löst Resync aus
    Res rs = r.parser.feed('/');
    assertResult(Res::Dropped, rs);
    assertDrop(Drop::Resync, r.parser.lastDropReason());

    // der Rest des zweiten Telegramms: genau ein Complete, nur eigener Inhalt
    FeedStats s2 = feedAll(r.parser, second.substr(1));
    TEST_ASSERT_EQUAL_INT(1, s2.complete);
    TEST_ASSERT_EQUAL_INT(0, s2.dropped);
    assertStr(second, dataOf(r.parser));
}

static void test_resync_full_stream_positions()
{
    Rig r;
    std::string first = "/A\r\n1-0:1.8.0(1*kWh)\r\n";
    std::string second = "/B\r\n1-0:16.7.0(5*W)\r\n!\r\n";
    FeedStats st = feedAll(r.parser, first + second);
    TEST_ASSERT_EQUAL_INT(1, st.dropped);
    TEST_ASSERT_EQUAL_INT((long)first.size(), st.droppedAt);
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    assertDrop(Drop::Resync, r.parser.lastDropReason());
    assertStr("/B\r\n1-0:16.7.0(5*W)\r\n!", dataOf(r.parser));
}

static void test_resync_with_lf_only_and_7e1()
{
    Rig r;
    std::string stream = encode7E1("/A\n1-0:1.8.0(1*kWh)\n/B\n1-0:16.7.0(5*W)\n!\n");
    FeedStats st = feedAll(r.parser, stream);
    TEST_ASSERT_EQUAL_INT(1, st.dropped);
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    assertDrop(Drop::Resync, r.parser.lastDropReason());
    assertStr("/B\n1-0:16.7.0(5*W)\n!", dataOf(r.parser));
}

static void test_garbage_lines_do_not_affect_framing()
{
    Rig r;
    std::string tel = join({"/LOG5LK13BE803039", "1-0:96.1.0*255(001LOG0067059477)", "#garbage-line#",
                            "1-0:1.8.0*255(021915.9796*kWh", "1-0:16.7.0*255(000696*W)", "!"},
                           "\r\n");
    FeedStats st = feedAll(r.parser, tel + "\r\n");
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_EQUAL_INT(0, st.dropped);
    assertStr(tel, dataOf(r.parser));
}

// ---------------------------------------------------------------- Overflow

static void test_overflow_drops_exactly_once_then_recovers()
{
    Rig r(64);
    std::string tel = sample("\r\n");
    TEST_ASSERT_TRUE(tel.size() > 64);
    FeedStats st = feedAll(r.parser, tel + "\r\n");
    TEST_ASSERT_EQUAL_INT(1, st.dropped);
    TEST_ASSERT_EQUAL_INT(0, st.complete);
    // Das 65. Byte (Index 64) sprengt die Kapazität.
    TEST_ASSERT_EQUAL_INT(64, st.droppedAt);
    assertDrop(Drop::Overflow, r.parser.lastDropReason());

    // danach: Idle, ein kleines Telegramm passt wieder
    std::string small = "/X\r\n1.8.0(1*kWh)\r\n!";
    FeedStats st2 = feedAll(r.parser, small);
    TEST_ASSERT_EQUAL_INT(1, st2.complete);
    TEST_ASSERT_EQUAL_INT(0, st2.dropped);
    assertStr(small, dataOf(r.parser));
}

static void test_overflow_boundary_exact_capacity_fits()
{
    // Annahme: kein Platz für ein NUL reserviert - ein Telegramm von exakt `capacity` Bytes passt.
    std::string tel = "/X\r\n!"; // 5 Bytes
    {
        Rig r(5);
        FeedStats st = feedAll(r.parser, tel);
        TEST_ASSERT_EQUAL_INT(1, st.complete);
        TEST_ASSERT_EQUAL_INT(0, st.dropped);
        assertStr(tel, dataOf(r.parser));
    }
    {
        Rig r(4);
        FeedStats st = feedAll(r.parser, tel);
        TEST_ASSERT_EQUAL_INT(0, st.complete);
        TEST_ASSERT_EQUAL_INT(1, st.dropped);
        TEST_ASSERT_EQUAL_INT(4, st.droppedAt);
        assertDrop(Drop::Overflow, r.parser.lastDropReason());
    }
}

// ---------------------------------------------------------------- BCC (STX/ETX)

static void test_bcc_correct_completes()
{
    Rig r;
    std::string expected;
    std::string raw = bccStream("/LOG5LK13BE803039\r\n", "1-0:1.8.0*255(021915.9796*kWh)\r\n1-0:16.7.0*255(000696*W)\r\n", "\r\n", &expected);
    FeedStats st = feedAll(r.parser, raw);
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_EQUAL_INT(0, st.dropped);
    // Complete erst auf dem BCC-Byte (letztes Byte)
    TEST_ASSERT_EQUAL_INT((long)raw.size() - 1, st.completeAt);
    TEST_ASSERT_TRUE(r.parser.bccChecked());
    assertStr(expected, dataOf(r.parser));
    // weder STX noch ETX noch BCC in den Daten
    TEST_ASSERT_TRUE(dataOf(r.parser).find(STX) == std::string::npos);
    TEST_ASSERT_TRUE(dataOf(r.parser).find(ETX) == std::string::npos);
}

static void test_bcc_correct_lf_only_tail()
{
    Rig r;
    std::string expected;
    std::string raw = bccStream("/A\n", "1-0:1.8.0(1*kWh)\n", "\n", &expected);
    FeedStats st = feedAll(r.parser, raw);
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_TRUE(r.parser.bccChecked());
    assertStr(expected, dataOf(r.parser));
}

static void test_bcc_full_7e1_encoded_including_bcc_byte()
{
    Rig r;
    std::string expected;
    std::string raw = bccStream("/LOG5LK13BE803039\r\n", "1-0:1.8.0*255(021915.9796*kWh)\r\n1-0:16.7.0*255(000696*W)\r\n", "\r\n", &expected);
    std::string coded = encode7E1(raw);
    TEST_ASSERT_TRUE(coded != raw);
    FeedStats st = feedAll(r.parser, coded);
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_EQUAL_INT(0, st.dropped);
    TEST_ASSERT_EQUAL_INT((long)coded.size() - 1, st.completeAt);
    TEST_ASSERT_TRUE(r.parser.bccChecked());
    assertStr(expected, dataOf(r.parser));
}

static void test_bcc_mismatch_drops()
{
    Rig r;
    std::string raw = bccStream("/A\r\n", "1-0:1.8.0(1*kWh)\r\n", "\r\n", nullptr, 0x01);
    FeedStats st = feedAll(r.parser, raw);
    TEST_ASSERT_EQUAL_INT(0, st.complete);
    TEST_ASSERT_EQUAL_INT(1, st.dropped);
    TEST_ASSERT_EQUAL_INT((long)raw.size() - 1, st.droppedAt);
    assertDrop(Drop::BccMismatch, r.parser.lastDropReason());
}

static void test_bcc_mismatch_then_parser_is_idle_and_recovers()
{
    Rig r;
    feedAll(r.parser, bccStream("/A\r\n", "1-0:1.8.0(1*kWh)\r\n", "\r\n", nullptr, 0x40));
    assertDrop(Drop::BccMismatch, r.parser.lastDropReason());
    std::string tel = "/B\r\n1-0:16.7.0(5*W)\r\n!";
    FeedStats st = feedAll(r.parser, tel);
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_FALSE(r.parser.bccChecked());
    assertStr(tel, dataOf(r.parser));
}

static void test_bcc_other_byte_between_bang_and_etx_is_framing_error()
{
    Rig r;
    std::string head = "/A\r\n";
    std::string body = "1-0:1.8.0(1*kWh)\r\n";
    // 'X' zwischen '!' und ETX
    std::string raw = head + std::string(1, STX) + body + "!\r\nX" + std::string(1, ETX) + "?";
    FeedStats st = feedAll(r.parser, raw);
    TEST_ASSERT_EQUAL_INT(0, st.complete);
    TEST_ASSERT_EQUAL_INT(1, st.dropped);
    assertDrop(Drop::Framing, r.parser.lastDropReason());
    TEST_ASSERT_EQUAL_INT((long)(head.size() + 1 + body.size() + 3), st.droppedAt); // Position des 'X'
}

static void test_bcc_slash_between_bang_and_etx_resyncs_and_restarts()
{
    Rig r;
    std::string raw = std::string("/A\r\n") + STX + "1-0:1.8.0(1*kWh)\r\n!\r\n/";
    FeedStats st = feedAll(r.parser, raw);
    TEST_ASSERT_EQUAL_INT(0, st.complete);
    TEST_ASSERT_EQUAL_INT(1, st.dropped);
    TEST_ASSERT_EQUAL_INT((long)raw.size() - 1, st.droppedAt);
    assertDrop(Drop::Resync, r.parser.lastDropReason());

    // Das '/' hat ein neues Telegramm gestartet (ohne STX): Complete auf dem '!'.
    FeedStats st2 = feedAll(r.parser, "Y\r\n!");
    TEST_ASSERT_EQUAL_INT(1, st2.complete);
    TEST_ASSERT_FALSE(r.parser.bccChecked());
    assertStr("/Y\r\n!", dataOf(r.parser));
}

static void test_bcc_truncated_never_completes()
{
    // ohne BCC-Byte
    std::string raw = bccStream("/A\r\n", "1-0:1.8.0(1*kWh)\r\n", "\r\n");
    Rig r;
    FeedStats st = feedAll(r.parser, raw.substr(0, raw.size() - 1));
    TEST_ASSERT_EQUAL_INT(0, st.complete);
    TEST_ASSERT_EQUAL_INT(0, st.dropped);
    // ohne ETX
    Rig r2;
    FeedStats st2 = feedAll(r2.parser, raw.substr(0, raw.size() - 2));
    TEST_ASSERT_EQUAL_INT(0, st2.complete);
    TEST_ASSERT_EQUAL_INT(0, st2.dropped);
}

static void test_bcc_checked_flag_follows_last_complete()
{
    Rig r;
    feedAll(r.parser, bccStream("/A\r\n", "1-0:1.8.0(1*kWh)\r\n", "\r\n"));
    TEST_ASSERT_TRUE(r.parser.bccChecked());
    FeedStats st = feedAll(r.parser, "/B\r\n1-0:16.7.0(5*W)\r\n!");
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_FALSE(r.parser.bccChecked());
}

// ---------------------------------------------------------------- reset

static void test_reset_mid_telegram()
{
    Rig r;
    feedAll(r.parser, "/A\r\n1-0:1.8.0(1*kWh)\r\n");
    TEST_ASSERT_TRUE(r.parser.length() > 0);
    r.parser.reset();
    TEST_ASSERT_EQUAL_UINT(0, r.parser.length());
    assertDrop(Drop::None, r.parser.lastDropReason());
    // Idle: der Rest des alten Telegramms wird ignoriert
    FeedStats st = feedAll(r.parser, "1-0:16.7.0(5*W)\r\n!\r\n");
    TEST_ASSERT_EQUAL_INT(0, st.complete);
    TEST_ASSERT_EQUAL_INT(0, st.dropped);
    TEST_ASSERT_EQUAL_UINT(0, r.parser.length());
}

static void test_reset_clears_drop_reason_and_new_telegram_has_no_old_content()
{
    Rig r(16);
    feedAll(r.parser, sample("\r\n")); // Overflow
    assertDrop(Drop::Overflow, r.parser.lastDropReason());
    r.parser.reset();
    assertDrop(Drop::None, r.parser.lastDropReason());
    TEST_ASSERT_EQUAL_UINT(0, r.parser.length());
    FeedStats st = feedAll(r.parser, "/Z\r\n!");
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    assertStr("/Z\r\n!", dataOf(r.parser));
}

static void test_reset_after_complete()
{
    Rig r;
    feedAll(r.parser, bccStream("/A\r\n", "1-0:1.8.0(1*kWh)\r\n", "\r\n"));
    TEST_ASSERT_TRUE(r.parser.length() > 0);
    r.parser.reset();
    TEST_ASSERT_EQUAL_UINT(0, r.parser.length());
    // Parser steht wieder sauber auf Idle: ein Telegramm ohne STX liefert Complete ohne BCC
    FeedStats st = feedAll(r.parser, "/B\r\n!");
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_FALSE(r.parser.bccChecked());
    assertStr("/B\r\n!", dataOf(r.parser));
}

static void test_reset_in_bcc_tail_drops_stx_state()
{
    Rig r;
    // mitten im Tail (nach '!', vor ETX) zurücksetzen
    feedAll(r.parser, std::string("/A\r\n") + STX + "1-0:1.8.0(1*kWh)\r\n!\r\n");
    r.parser.reset();
    FeedStats st = feedAll(r.parser, "/B\r\n!");
    TEST_ASSERT_EQUAL_INT(1, st.complete);
    TEST_ASSERT_EQUAL_INT(0, st.dropped);
    TEST_ASSERT_FALSE(r.parser.bccChecked());
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_initial_state);
    RUN_TEST(test_idle_ignores_bytes_until_slash);
    RUN_TEST(test_sample_crlf_byte_by_byte);
    RUN_TEST(test_sample_lf_only);
    RUN_TEST(test_sample_7e1_same_data_as_plain);
    RUN_TEST(test_encode7e1_helper_really_sets_bit7);
    RUN_TEST(test_data_stays_valid_after_complete_until_next_feed);
    RUN_TEST(test_two_telegrams_back_to_back);
    RUN_TEST(test_bang_and_slash_only_matter_at_line_start);
    RUN_TEST(test_truncated_never_completes);
    RUN_TEST(test_truncated_mid_line_then_silence);
    RUN_TEST(test_resync_on_slash_at_line_start);
    RUN_TEST(test_resync_full_stream_positions);
    RUN_TEST(test_resync_with_lf_only_and_7e1);
    RUN_TEST(test_garbage_lines_do_not_affect_framing);
    RUN_TEST(test_overflow_drops_exactly_once_then_recovers);
    RUN_TEST(test_overflow_boundary_exact_capacity_fits);
    RUN_TEST(test_bcc_correct_completes);
    RUN_TEST(test_bcc_correct_lf_only_tail);
    RUN_TEST(test_bcc_full_7e1_encoded_including_bcc_byte);
    RUN_TEST(test_bcc_mismatch_drops);
    RUN_TEST(test_bcc_mismatch_then_parser_is_idle_and_recovers);
    RUN_TEST(test_bcc_other_byte_between_bang_and_etx_is_framing_error);
    RUN_TEST(test_bcc_slash_between_bang_and_etx_resyncs_and_restarts);
    RUN_TEST(test_bcc_truncated_never_completes);
    RUN_TEST(test_bcc_checked_flag_follows_last_complete);
    RUN_TEST(test_reset_mid_telegram);
    RUN_TEST(test_reset_clears_drop_reason_and_new_telegram_has_no_old_content);
    RUN_TEST(test_reset_after_complete);
    RUN_TEST(test_reset_in_bcc_tail_drops_stx_state);
    return UNITY_END();
}
