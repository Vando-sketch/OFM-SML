// Tests für SMLHelpers: Baudrate, Datenformat, Timeout, Empfangsstatus, ASCII-Erkennung, Infotext.
#include <cstdint>
#include <cstring>
#include <unity.h>

#include "SMLHelpers.h"

using namespace SMLHelpers;

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------- baudFromParam

static void test_baud_table()
{
    TEST_ASSERT_EQUAL_UINT32(9600, baudFromParam(0));
    TEST_ASSERT_EQUAL_UINT32(300, baudFromParam(1));
    TEST_ASSERT_EQUAL_UINT32(1200, baudFromParam(2));
    TEST_ASSERT_EQUAL_UINT32(2400, baudFromParam(3));
    TEST_ASSERT_EQUAL_UINT32(4800, baudFromParam(4));
    TEST_ASSERT_EQUAL_UINT32(19200, baudFromParam(5));
}

static void test_baud_unknown_defaults_to_9600()
{
    TEST_ASSERT_EQUAL_UINT32(9600, baudFromParam(6));
    TEST_ASSERT_EQUAL_UINT32(9600, baudFromParam(7));
    TEST_ASSERT_EQUAL_UINT32(9600, baudFromParam(255));
}

// ---------------------------------------------------------------- formatFromParams

static void test_format_explicit()
{
    TEST_ASSERT_TRUE(formatFromParams(0, 1) == SerialFormat::F8N1);
    TEST_ASSERT_TRUE(formatFromParams(1, 1) == SerialFormat::F8N1);
    TEST_ASSERT_TRUE(formatFromParams(0, 2) == SerialFormat::F7E1);
    TEST_ASSERT_TRUE(formatFromParams(1, 2) == SerialFormat::F7E1);
}

static void test_format_auto_depends_on_protocol()
{
    TEST_ASSERT_TRUE(formatFromParams(0, 0) == SerialFormat::F8N1);
    TEST_ASSERT_TRUE(formatFromParams(1, 0) == SerialFormat::F7E1);
}

static void test_format_unknown_dataformat_acts_like_auto()
{
    TEST_ASSERT_TRUE(formatFromParams(0, 3) == SerialFormat::F8N1);
    TEST_ASSERT_TRUE(formatFromParams(1, 3) == SerialFormat::F7E1);
    TEST_ASSERT_TRUE(formatFromParams(0, 255) == SerialFormat::F8N1);
    TEST_ASSERT_TRUE(formatFromParams(1, 255) == SerialFormat::F7E1);
}

// ---------------------------------------------------------------- Namen

static void test_names()
{
    TEST_ASSERT_EQUAL_STRING("8N1", formatName(SerialFormat::F8N1));
    TEST_ASSERT_EQUAL_STRING("7E1", formatName(SerialFormat::F7E1));
    TEST_ASSERT_EQUAL_STRING("SML", protocolName(0));
    TEST_ASSERT_EQUAL_STRING("ASCII", protocolName(1));
}

static void test_protocol_name_unknown_is_sml()
{
    // Alles außer 1 verhält sich wie SML (siehe isAscii() im Kanal)
    TEST_ASSERT_EQUAL_STRING("SML", protocolName(2));
    TEST_ASSERT_EQUAL_STRING("SML", protocolName(255));
}

// ---------------------------------------------------------------- timeoutMsFromParam

static void test_timeout_zero_defaults()
{
    TEST_ASSERT_EQUAL_UINT32(5000, timeoutMsFromParam(0));
}

static void test_timeout_seconds_to_ms()
{
    TEST_ASSERT_EQUAL_UINT32(1000, timeoutMsFromParam(1));
    TEST_ASSERT_EQUAL_UINT32(5000, timeoutMsFromParam(5));
    TEST_ASSERT_EQUAL_UINT32(255000, timeoutMsFromParam(255));
}

// ---------------------------------------------------------------- isReceiving

static void test_receiving_never_received()
{
    TEST_ASSERT_FALSE(isReceiving(0, 0, 5000));
    TEST_ASSERT_FALSE(isReceiving(1000, 0, 5000));
    TEST_ASSERT_FALSE(isReceiving(0xFFFFFFFFu, 0, 5000));
}

static void test_receiving_within_and_after_timeout()
{
    TEST_ASSERT_TRUE(isReceiving(1000, 1000, 5000));
    TEST_ASSERT_TRUE(isReceiving(5999, 1000, 5000));
    TEST_ASSERT_FALSE(isReceiving(6000, 1000, 5000)); // genau am Timeout: abgelaufen
    TEST_ASSERT_FALSE(isReceiving(60000, 1000, 5000));
}

static void test_receiving_millis_wraparound()
{
    // Telegramm 1 s vor dem Überlauf, "jetzt" 2 s danach: 3 s vergangen
    const uint32_t last = 0xFFFFFFFFu - 999u;
    TEST_ASSERT_TRUE(isReceiving(2000, last, 5000));
    // 6 s vergangen über den Überlauf hinweg
    TEST_ASSERT_FALSE(isReceiving(5000, last, 5000));
    TEST_ASSERT_TRUE(isReceiving(3998, last, 5000));
    TEST_ASSERT_FALSE(isReceiving(4000, last, 5000));
}

static void test_receiving_last_valid_at_max()
{
    TEST_ASSERT_TRUE(isReceiving(0xFFFFFFFFu, 0xFFFFFFFFu, 5000));
    TEST_ASSERT_TRUE(isReceiving(0, 0xFFFFFFFFu, 5000));
}

// ---------------------------------------------------------------- looksLikeObisAscii

static bool like(const char *s)
{
    return looksLikeObisAscii((const uint8_t *)s, strlen(s));
}

static void test_ascii_detect_plain()
{
    TEST_ASSERT_TRUE(like("/LOG5LK13BE803097\r\n"));
    TEST_ASSERT_TRUE(like("/LOG5LK1")); // genau 8 Bytes
}

static void test_ascii_detect_too_short()
{
    TEST_ASSERT_FALSE(like("/LOG5LK"));
    TEST_ASSERT_FALSE(like("/"));
    TEST_ASSERT_FALSE(like(""));
    TEST_ASSERT_FALSE(looksLikeObisAscii(nullptr, 0));
}

static void test_ascii_detect_wrong_start()
{
    TEST_ASSERT_FALSE(like("XLOG5LK13BE"));
    TEST_ASSERT_FALSE(like(" /LOG5LK13BE"));
}

static void test_ascii_detect_7e1_encoded()
{
    // "/LOG5LK1" mit gerader Parität (Bit 7 gesetzt, wenn die Zahl der 1-Bits ungerade ist)
    const char *text = "/LOG5LK1";
    uint8_t buf[8];
    for (int i = 0; i < 8; i++)
    {
        uint8_t c = (uint8_t)text[i];
        uint8_t ones = 0;
        for (int b = 0; b < 7; b++)
            ones += (c >> b) & 1;
        buf[i] = (ones & 1) ? (c | 0x80) : c;
    }
    // Mindestens ein Byte muss tatsächlich Bit 7 gesetzt haben, sonst prüft der Test nichts
    bool anyHigh = false;
    for (int i = 0; i < 8; i++)
        anyHigh |= (buf[i] & 0x80) != 0;
    TEST_ASSERT_TRUE(anyHigh);
    TEST_ASSERT_TRUE(looksLikeObisAscii(buf, sizeof(buf)));
}

static void test_ascii_detect_sml_start_sequence()
{
    const uint8_t sml[8] = {0x1B, 0x1B, 0x1B, 0x1B, 0x01, 0x01, 0x01, 0x01};
    TEST_ASSERT_FALSE(looksLikeObisAscii(sml, sizeof(sml)));
}

static void test_ascii_detect_binary_garbage()
{
    const uint8_t bin[10] = {'/', 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    TEST_ASSERT_FALSE(looksLikeObisAscii(bin, sizeof(bin)));
    // Steuerzeichen mitten in den ersten acht Bytes
    const uint8_t ctrl[8] = {'/', 'L', 'O', 0x07, '5', 'L', 'K', '1'};
    TEST_ASSERT_FALSE(looksLikeObisAscii(ctrl, sizeof(ctrl)));
    // DEL (0x7F) ist nicht druckbar
    const uint8_t del[8] = {'/', 'L', 'O', 0x7F, '5', 'L', 'K', '1'};
    TEST_ASSERT_FALSE(looksLikeObisAscii(del, sizeof(del)));
}

static void test_ascii_detect_crlf_allowed()
{
    const uint8_t t[8] = {'/', 'A', 'B', '\r', '\n', 'C', 'D', 'E'};
    TEST_ASSERT_TRUE(looksLikeObisAscii(t, sizeof(t)));
}

static void test_ascii_detect_only_first_eight_bytes_checked()
{
    const uint8_t t[10] = {'/', 'L', 'O', 'G', '5', 'L', 'K', '1', 0x00, 0x01};
    TEST_ASSERT_TRUE(looksLikeObisAscii(t, sizeof(t)));
}

// ---------------------------------------------------------------- formatSerialInfo

static void test_info_ascii()
{
    char out[32];
    size_t n = formatSerialInfo(out, sizeof(out), 1, 9600, SerialFormat::F7E1);
    TEST_ASSERT_EQUAL_STRING("ASCII 9600 7E1", out);
    TEST_ASSERT_EQUAL_UINT32(14, n);
}

static void test_info_sml()
{
    char out[32];
    size_t n = formatSerialInfo(out, sizeof(out), 0, 9600, SerialFormat::F8N1);
    TEST_ASSERT_EQUAL_STRING("SML 9600 8N1", out);
    TEST_ASSERT_EQUAL_UINT32(12, n);
}

static void test_info_other_baud()
{
    char out[32];
    formatSerialInfo(out, sizeof(out), 1, 300, SerialFormat::F7E1);
    TEST_ASSERT_EQUAL_STRING("ASCII 300 7E1", out);
    formatSerialInfo(out, sizeof(out), 0, 19200, SerialFormat::F8N1);
    TEST_ASSERT_EQUAL_STRING("SML 19200 8N1", out);
}

static void test_info_truncation()
{
    char out[8];
    memset(out, 'Z', sizeof(out));
    size_t n = formatSerialInfo(out, sizeof(out), 1, 9600, SerialFormat::F7E1);
    TEST_ASSERT_EQUAL_STRING("ASCII 9", out); // 7 Zeichen + NUL
    TEST_ASSERT_EQUAL_UINT32(7, n);
}

static void test_info_exact_fit_and_one_short()
{
    char out[15];
    size_t n = formatSerialInfo(out, sizeof(out), 1, 9600, SerialFormat::F7E1); // 14 Zeichen + NUL
    TEST_ASSERT_EQUAL_UINT32(14, n);
    TEST_ASSERT_EQUAL_STRING("ASCII 9600 7E1", out);

    char shortBuf[14];
    n = formatSerialInfo(shortBuf, sizeof(shortBuf), 1, 9600, SerialFormat::F7E1);
    TEST_ASSERT_EQUAL_UINT32(13, n);
    TEST_ASSERT_EQUAL_STRING("ASCII 9600 7E", shortBuf);
}

static void test_info_tiny_buffers()
{
    char one[1] = {'Z'};
    TEST_ASSERT_EQUAL_UINT32(0, formatSerialInfo(one, 1, 0, 9600, SerialFormat::F8N1));
    TEST_ASSERT_EQUAL_CHAR('\0', one[0]);

    char two[2];
    TEST_ASSERT_EQUAL_UINT32(1, formatSerialInfo(two, 2, 0, 9600, SerialFormat::F8N1));
    TEST_ASSERT_EQUAL_STRING("S", two);

    // outSize 0: nichts schreiben, nichts crashen
    char guard[2] = {'Z', 'Z'};
    TEST_ASSERT_EQUAL_UINT32(0, formatSerialInfo(guard, 0, 0, 9600, SerialFormat::F8N1));
    TEST_ASSERT_EQUAL_CHAR('Z', guard[0]);
    TEST_ASSERT_EQUAL_UINT32(0, formatSerialInfo(nullptr, 0, 0, 9600, SerialFormat::F8N1));
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_baud_table);
    RUN_TEST(test_baud_unknown_defaults_to_9600);
    RUN_TEST(test_format_explicit);
    RUN_TEST(test_format_auto_depends_on_protocol);
    RUN_TEST(test_format_unknown_dataformat_acts_like_auto);
    RUN_TEST(test_names);
    RUN_TEST(test_protocol_name_unknown_is_sml);
    RUN_TEST(test_timeout_zero_defaults);
    RUN_TEST(test_timeout_seconds_to_ms);
    RUN_TEST(test_receiving_never_received);
    RUN_TEST(test_receiving_within_and_after_timeout);
    RUN_TEST(test_receiving_millis_wraparound);
    RUN_TEST(test_receiving_last_valid_at_max);
    RUN_TEST(test_ascii_detect_plain);
    RUN_TEST(test_ascii_detect_too_short);
    RUN_TEST(test_ascii_detect_wrong_start);
    RUN_TEST(test_ascii_detect_7e1_encoded);
    RUN_TEST(test_ascii_detect_sml_start_sequence);
    RUN_TEST(test_ascii_detect_binary_garbage);
    RUN_TEST(test_ascii_detect_crlf_allowed);
    RUN_TEST(test_ascii_detect_only_first_eight_bytes_checked);
    RUN_TEST(test_info_ascii);
    RUN_TEST(test_info_sml);
    RUN_TEST(test_info_other_baud);
    RUN_TEST(test_info_truncation);
    RUN_TEST(test_info_exact_fit_and_one_short);
    RUN_TEST(test_info_tiny_buffers);
    return UNITY_END();
}
