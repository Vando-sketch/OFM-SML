// Tests für ObisAscii::normalizeUnit und ObisAscii::trimMeterId.
#include "../support/ObisTestSupport.h"

using namespace TestSupport;

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------- normalizeUnit

static void check(double in, const char *unit, double expectedValue, const char *expectedUnit)
{
    char out[16];
    memset(out, 'Z', sizeof(out));
    double v = ObisAscii::normalizeUnit(in, unit, out, sizeof(out));
    assertNear(expectedValue, v);
    TEST_ASSERT_EQUAL_STRING(expectedUnit, out);
}

static void test_kwh_to_wh()
{
    check(21915.9796, "kWh", 21915979.6, "Wh");
    check(0.5, "kWh", 500, "Wh");
    check(0, "kWh", 0, "Wh");
}

static void test_kw_to_w()
{
    check(1.5, "kW", 1500, "W");
}

static void test_case_insensitive()
{
    check(2, "KWH", 2000, "Wh");
    check(2, "kwh", 2000, "Wh");
    check(2, "Kwh", 2000, "Wh");
    check(2, "kw", 2000, "W");
    check(2, "KW", 2000, "W");
}

static void test_other_units_unchanged()
{
    check(5, "W", 5, "W");
    check(5, "Wh", 5, "Wh");
    check(230.1, "V", 230.1, "V");
    check(1.2, "A", 1.2, "A");
    check(50.01, "Hz", 50.01, "Hz");
    check(7, "", 7, "");
    check(7, "kvarh", 7, "kvarh");
}

static void test_out_unit_truncation_and_termination()
{
    char out[8];

    // kWh -> "Wh" in 2 Bytes: gekürzt auf "W", Wert trotzdem umgerechnet
    memset(out, 'Z', sizeof(out));
    double v = ObisAscii::normalizeUnit(1, "kWh", out, 2);
    assertNear(1000, v);
    TEST_ASSERT_EQUAL_STRING("W", out);
    TEST_ASSERT_EQUAL_INT('Z', out[2]); // nichts hinter outUnitSize geschrieben

    // unveränderte Einheit wird gekürzt kopiert
    memset(out, 'Z', sizeof(out));
    v = ObisAscii::normalizeUnit(3, "kvarh", out, 4);
    assertNear(3, v);
    TEST_ASSERT_EQUAL_STRING("kva", out);
    TEST_ASSERT_EQUAL_INT('Z', out[4]);

    // outUnitSize 1: nur das Terminator-Byte
    memset(out, 'Z', sizeof(out));
    ObisAscii::normalizeUnit(3, "Hz", out, 1);
    TEST_ASSERT_EQUAL_INT(0, out[0]);
    TEST_ASSERT_EQUAL_INT('Z', out[1]);

    // exakt passend
    memset(out, 'Z', sizeof(out));
    ObisAscii::normalizeUnit(3, "Hz", out, 3);
    TEST_ASSERT_EQUAL_STRING("Hz", out);
}

// ---------------------------------------------------------------- trimMeterId

static size_t trim(const char *in, size_t outSize, std::string &result, size_t len = (size_t)-1)
{
    std::vector<char> out(outSize + 2, 'Z');
    if (len == (size_t)-1)
        len = strlen(in);
    size_t n = ObisAscii::trimMeterId(in, len, out.data(), outSize);
    result = std::string(out.data());
    // Wächter hinter dem Puffer darf nicht überschrieben sein
    TEST_ASSERT_EQUAL_INT('Z', out[outSize]);
    TEST_ASSERT_EQUAL_INT('Z', out[outSize + 1]);
    return n;
}

static void test_trim_strips_leading_zeros_down_to_14()
{
    std::string r;
    TEST_ASSERT_EQUAL_UINT(14, trim("001LOG0067059477", 32, r));
    assertStr("1LOG0067059477", r);
}

static void test_trim_stops_at_14_chars()
{
    std::string r;
    // 18 Zeichen -> auf 14 gekürzt, auch wenn dann noch eine '0' vorne steht
    TEST_ASSERT_EQUAL_UINT(14, trim("0000" "12345678901234", 32, r));
    assertStr("12345678901234", r);
    TEST_ASSERT_EQUAL_UINT(14, trim("0000" "01234567890123", 32, r));
    assertStr("01234567890123", r);
    // nur Nullen: bleibt bei 14
    TEST_ASSERT_EQUAL_UINT(14, trim("0000000000000000000000", 32, r));
    assertStr("00000000000000", r);
    // 15 Zeichen: genau eine Null entfällt
    TEST_ASSERT_EQUAL_UINT(14, trim("012345678901234", 32, r));
    assertStr("12345678901234", r);
}

static void test_trim_leaves_short_ids_unchanged()
{
    std::string r;
    TEST_ASSERT_EQUAL_UINT(14, trim("00000000000001", 32, r));
    assertStr("00000000000001", r);
    TEST_ASSERT_EQUAL_UINT(8, trim("0LOG1234", 32, r));
    assertStr("0LOG1234", r);
    TEST_ASSERT_EQUAL_UINT(14, trim("1LOG0067059477", 32, r));
    assertStr("1LOG0067059477", r);
    TEST_ASSERT_EQUAL_UINT(0, trim("", 32, r));
    assertStr("", r);
}

static void test_trim_keeps_long_ids_without_leading_zero()
{
    std::string r;
    TEST_ASSERT_EQUAL_UINT(16, trim("1234567890ABCDEF", 32, r));
    assertStr("1234567890ABCDEF", r);
    TEST_ASSERT_EQUAL_UINT(15, trim("123456789012345", 32, r));
    assertStr("123456789012345", r);
}

static void test_trim_only_leading_zeros_are_stripped()
{
    std::string r;
    TEST_ASSERT_EQUAL_UINT(17, trim("1000000000000000A", 32, r)); // 17 Zeichen, keine führende Null
    assertStr("1000000000000000A", r);
}

static void test_trim_truncates_to_out_size()
{
    std::string r;
    TEST_ASSERT_EQUAL_UINT(4, trim("001LOG0067059477", 5, r));
    assertStr("1LOG", r);
    TEST_ASSERT_EQUAL_UINT(0, trim("001LOG0067059477", 1, r));
    assertStr("", r);
    // exakt passend inkl. Terminator
    TEST_ASSERT_EQUAL_UINT(14, trim("001LOG0067059477", 15, r));
    assertStr("1LOG0067059477", r);
    // unveränderte lange ID wird ebenfalls gekürzt
    TEST_ASSERT_EQUAL_UINT(9, trim("1234567890ABCDEF", 10, r));
    assertStr("123456789", r);
}

static void test_trim_honours_len_not_nul()
{
    std::string r;
    TEST_ASSERT_EQUAL_UINT(14, trim("001LOG0067059477XYZ", 32, r, 16));
    assertStr("1LOG0067059477", r);
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_kwh_to_wh);
    RUN_TEST(test_kw_to_w);
    RUN_TEST(test_case_insensitive);
    RUN_TEST(test_other_units_unchanged);
    RUN_TEST(test_out_unit_truncation_and_termination);
    RUN_TEST(test_trim_strips_leading_zeros_down_to_14);
    RUN_TEST(test_trim_stops_at_14_chars);
    RUN_TEST(test_trim_leaves_short_ids_unchanged);
    RUN_TEST(test_trim_keeps_long_ids_without_leading_zero);
    RUN_TEST(test_trim_only_leading_zeros_are_stripped);
    RUN_TEST(test_trim_truncates_to_out_size);
    RUN_TEST(test_trim_honours_len_not_nul);
    return UNITY_END();
}
