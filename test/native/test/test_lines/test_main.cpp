// Tests für ObisAscii::parseLine (einzelne Zeile ohne CR/LF).
#include "../support/ObisTestSupport.h"

using namespace TestSupport;
using ObisAscii::Entry;
using ObisAscii::LineKind;

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------- Zeilenarten

static void test_empty_line()
{
    Entry e;
    assertKind(LineKind::Empty, ObisAscii::parseLine("", 0, e));
}

static void test_ident_line()
{
    Entry e;
    assertKind(LineKind::Ident, parse("/LOG5LK13BE803039", e));
    assertKind(LineKind::Ident, parse("/", e));
}

static void test_end_line()
{
    Entry e;
    assertKind(LineKind::End, parse("!", e));
}

static void test_bang_with_more_is_invalid()
{
    Entry e;
    assertKind(LineKind::Invalid, parse("!x", e));
    assertKind(LineKind::Invalid, parse("! ", e));
}

static void test_non_data_kinds_leave_out_untouched()
{
    // Header: "Nur bei Data wird `out` befüllt."
    const char *lines[] = {"", "/LOG5", "!", "#garbage-line#", "1-0:1.8.0*255(021915.9796*kWh", "1.8.0"};
    for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++)
    {
        Entry e;
        memset(&e, 0xA5, sizeof(e));
        Entry ref;
        memset(&ref, 0xA5, sizeof(ref));
        LineKind k = ObisAscii::parseLine(lines[i], strlen(lines[i]), e);
        TEST_ASSERT_TRUE(k != LineKind::Data);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, memcmp(&e, &ref, sizeof(e)), lines[i]);
    }
}

// ---------------------------------------------------------------- Datenformen

static void test_full_form_with_unit()
{
    Entry e;
    assertKind(LineKind::Data, parse("1-0:1.8.0*255(021915.9796*kWh)", e));
    assertEntry(e, 1, 0, 1, 8, 0, 255, '\0', "021915.9796", "kWh", true, 21915.9796);
}

static void test_full_form_without_unit_and_non_numeric()
{
    Entry e;
    assertKind(LineKind::Data, parse("1-0:96.1.0*255(001LOG0067059477)", e));
    assertEntry(e, 1, 0, 96, 1, 0, 255, '\0', "001LOG0067059477", "", false, 0);
}

static void test_full_form_integer_value()
{
    Entry e;
    assertKind(LineKind::Data, parse("1-0:16.7.0*255(000696*W)", e));
    assertEntry(e, 1, 0, 16, 7, 0, 255, '\0', "000696", "W", true, 696);
}

static void test_full_form_text_value()
{
    Entry e;
    assertKind(LineKind::Data, parse("1-0:0.2.0*255(ver.03,432F,20170504)", e));
    assertEntry(e, 1, 0, 0, 2, 0, 255, '\0', "ver.03,432F,20170504", "", false, 0);
}

static void test_full_form_without_f_defaults_to_255()
{
    Entry e;
    assertKind(LineKind::Data, parse("1-0:1.8.0(1234.5*kWh)", e));
    assertEntry(e, 1, 0, 1, 8, 0, 255, '\0', "1234.5", "kWh", true, 1234.5);
}

static void test_full_form_other_a_b_and_f()
{
    Entry e;
    assertKind(LineKind::Data, parse("2-3:4.5.6*7(8*V)", e));
    assertEntry(e, 2, 3, 4, 5, 6, 7, '\0', "8", "V", true, 8);
}

static void test_short_form()
{
    Entry e;
    assertKind(LineKind::Data, parse("1.8.0(000123.4*kWh)", e));
    assertEntry(e, 1, 0, 1, 8, 0, 255, '\0', "000123.4", "kWh", true, 123.4);
}

static void test_short_form_with_f()
{
    Entry e;
    assertKind(LineKind::Data, parse("16.7.0*255(42*W)", e));
    assertEntry(e, 1, 0, 16, 7, 0, 255, '\0', "42", "W", true, 42);

    assertKind(LineKind::Data, parse("1.8.0*3(1*kWh)", e));
    assertEntry(e, 1, 0, 1, 8, 0, 3, '\0', "1", "kWh", true, 1);
}

static void test_group_limits()
{
    Entry e;
    assertKind(LineKind::Data, parse("255-255:255.255.255*255(1)", e));
    assertEntry(e, 255, 255, 255, 255, 255, 255, '\0', "1", "", true, 1);
    assertKind(LineKind::Data, parse("0-0:0.0.0*0(1)", e));
    assertEntry(e, 0, 0, 0, 0, 0, 0, '\0', "1", "", true, 1);
}

static void test_group_over_255_is_invalid()
{
    Entry e;
    assertKind(LineKind::Invalid, parse("256-0:1.8.0(1)", e));
    assertKind(LineKind::Invalid, parse("1-256:1.8.0(1)", e));
    assertKind(LineKind::Invalid, parse("1-0:256.8.0(1)", e));
    assertKind(LineKind::Invalid, parse("1-0:1.256.0(1)", e));
    assertKind(LineKind::Invalid, parse("1-0:1.8.256(1)", e));
    assertKind(LineKind::Invalid, parse("1-0:1.8.0*256(1)", e));
    assertKind(LineKind::Invalid, parse("1.8.300(1)", e));
}

// ---------------------------------------------------------------- Buchstabencodes

static void test_letter_code_short_form()
{
    Entry e;
    assertKind(LineKind::Data, parse("C.90.2(12345678)", e));
    assertEntry(e, 1, 0, 0, 90, 2, 255, 'C', "12345678", "", true, 12345678);
}

static void test_letter_code_full_form()
{
    Entry e;
    assertKind(LineKind::Data, parse("1-0:C.90.2*255(12345678)", e));
    assertEntry(e, 1, 0, 0, 90, 2, 255, 'C', "12345678", "", true, 12345678);
}

static void test_letter_code_other_letter_is_kept()
{
    Entry e;
    assertKind(LineKind::Data, parse("F.1.0(7)", e));
    assertEntry(e, 1, 0, 0, 1, 0, 255, 'F', "7", "", true, 7);
}

static void test_letter_f_f_is_invalid()
{
    Entry e;
    assertKind(LineKind::Invalid, parse("F.F(00)", e));
}

static void test_letters_outside_group_c_are_invalid()
{
    Entry e;
    assertKind(LineKind::Invalid, parse("A-0:1.8.0(1)", e));
    assertKind(LineKind::Invalid, parse("1-B:1.8.0(1)", e));
    assertKind(LineKind::Invalid, parse("1-0:1.B.0(1)", e));
    assertKind(LineKind::Invalid, parse("1-0:1.8.X(1)", e));
    assertKind(LineKind::Invalid, parse("1.8.0*Z(1)", e));
}

// ---------------------------------------------------------------- Wert / Einheit

static void test_value_splits_at_last_star()
{
    Entry e;
    assertKind(LineKind::Data, parse("1.8.0(a*b*kWh)", e));
    assertEntry(e, 1, 0, 1, 8, 0, 255, '\0', "a*b", "kWh", false, 0);
}

static void test_text_after_first_paren_is_ignored()
{
    Entry e;
    assertKind(LineKind::Data, parse("1.8.0(1*kWh)(2*kWh)", e));
    assertEntry(e, 1, 0, 1, 8, 0, 255, '\0', "1", "kWh", true, 1);
    assertKind(LineKind::Data, parse("1.8.0(5)trailing", e));
    assertEntry(e, 1, 0, 1, 8, 0, 255, '\0', "5", "", true, 5);
}

static void test_len_parameter_is_honoured()
{
    // Nicht NUL-terminiert an der Zeilengrenze: nur die ersten `len` Zeichen zählen.
    const char *buf = "1.8.0(1)(2*kWh)";
    Entry e;
    assertKind(LineKind::Data, ObisAscii::parseLine(buf, 8, e));
    assertEntry(e, 1, 0, 1, 8, 0, 255, '\0', "1", "", true, 1);
    // abgeschnitten vor ')' -> Invalid
    assertKind(LineKind::Invalid, ObisAscii::parseLine(buf, 7, e));
}

static void test_numeric_forms()
{
    Entry e;
    assertKind(LineKind::Data, parse("1.8.0(-12.5)", e));
    assertEntry(e, 1, 0, 1, 8, 0, 255, '\0', "-12.5", "", true, -12.5);
    assertKind(LineKind::Data, parse("1.8.0(+7)", e));
    assertEntry(e, 1, 0, 1, 8, 0, 255, '\0', "+7", "", true, 7);
    assertKind(LineKind::Data, parse("1.8.0(000696)", e));
    assertEntry(e, 1, 0, 1, 8, 0, 255, '\0', "000696", "", true, 696);
    assertKind(LineKind::Data, parse("1.8.0(0)", e));
    assertEntry(e, 1, 0, 1, 8, 0, 255, '\0', "0", "", true, 0);
}

static void test_non_numeric_forms()
{
    const char *values[] = {"12.", ".5", "1.2.3", "1e5", "--1", "12a", "0x10", "1,5", "ver.03", "+", "-", "."};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        std::string line = std::string("1.8.0(") + values[i] + ")";
        Entry e;
        assertKind(LineKind::Data, parse(line, e));
        TEST_ASSERT_FALSE_MESSAGE(e.numeric, values[i]);
        assertNear(0, e.number);
        TEST_ASSERT_EQUAL_STRING(values[i], e.value);
    }
}

static void test_value_length_limit()
{
    Entry e;
    std::string v40(ObisAscii::MaxValueLen, '1');
    assertKind(LineKind::Data, parse("1.8.0(" + v40 + ")", e));
    TEST_ASSERT_EQUAL_STRING(v40.c_str(), e.value);

    std::string v41(ObisAscii::MaxValueLen + 1, '1');
    assertKind(LineKind::Invalid, parse("1.8.0(" + v41 + ")", e));
    assertKind(LineKind::Invalid, parse("1.8.0(" + v41 + "*kWh)", e));
}

static void test_unit_length_limit()
{
    Entry e;
    std::string u8(ObisAscii::MaxUnitLen, 'x');
    assertKind(LineKind::Data, parse("1.8.0(1*" + u8 + ")", e));
    TEST_ASSERT_EQUAL_STRING(u8.c_str(), e.unit);

    std::string u9(ObisAscii::MaxUnitLen + 1, 'x');
    assertKind(LineKind::Invalid, parse("1.8.0(1*" + u9 + ")", e));
}

static void test_line_length_limit()
{
    Entry e;
    std::string base = "1.8.0(1)";
    // Rest nach der ersten ')' wird ignoriert, zählt aber zur Zeilenlänge.
    std::string ok = base + std::string(ObisAscii::MaxLineLen - base.size(), 'x');
    TEST_ASSERT_EQUAL_UINT(ObisAscii::MaxLineLen, ok.size());
    assertKind(LineKind::Data, parse(ok, e));
    assertEntry(e, 1, 0, 1, 8, 0, 255, '\0', "1", "", true, 1);

    std::string tooLong = ok + "x";
    assertKind(LineKind::Invalid, parse(tooLong, e));
}

// ---------------------------------------------------------------- Ungültiges

static void test_missing_paren_is_invalid()
{
    Entry e;
    assertKind(LineKind::Invalid, parse("1-0:1.8.0*255", e));
    assertKind(LineKind::Invalid, parse("1-0:1.8.0*255)", e));
    assertKind(LineKind::Invalid, parse("1.8.0 1234", e));
}

static void test_missing_closing_paren_is_invalid()
{
    Entry e;
    assertKind(LineKind::Invalid, parse("1-0:1.8.0*255(021915.9796*kWh", e));
    assertKind(LineKind::Invalid, parse("1.8.0(", e));
}

static void test_garbage_is_invalid()
{
    Entry e;
    assertKind(LineKind::Invalid, parse("#garbage-line#", e));
    assertKind(LineKind::Invalid, parse("hello world", e));
    assertKind(LineKind::Invalid, parse("(123)", e));
    assertKind(LineKind::Invalid, parse("1-0:(1)", e));
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_empty_line);
    RUN_TEST(test_ident_line);
    RUN_TEST(test_end_line);
    RUN_TEST(test_bang_with_more_is_invalid);
    RUN_TEST(test_non_data_kinds_leave_out_untouched);
    RUN_TEST(test_full_form_with_unit);
    RUN_TEST(test_full_form_without_unit_and_non_numeric);
    RUN_TEST(test_full_form_integer_value);
    RUN_TEST(test_full_form_text_value);
    RUN_TEST(test_full_form_without_f_defaults_to_255);
    RUN_TEST(test_full_form_other_a_b_and_f);
    RUN_TEST(test_short_form);
    RUN_TEST(test_short_form_with_f);
    RUN_TEST(test_group_limits);
    RUN_TEST(test_group_over_255_is_invalid);
    RUN_TEST(test_letter_code_short_form);
    RUN_TEST(test_letter_code_full_form);
    RUN_TEST(test_letter_code_other_letter_is_kept);
    RUN_TEST(test_letter_f_f_is_invalid);
    RUN_TEST(test_letters_outside_group_c_are_invalid);
    RUN_TEST(test_value_splits_at_last_star);
    RUN_TEST(test_text_after_first_paren_is_ignored);
    RUN_TEST(test_len_parameter_is_honoured);
    RUN_TEST(test_numeric_forms);
    RUN_TEST(test_non_numeric_forms);
    RUN_TEST(test_value_length_limit);
    RUN_TEST(test_unit_length_limit);
    RUN_TEST(test_line_length_limit);
    RUN_TEST(test_missing_paren_is_invalid);
    RUN_TEST(test_missing_closing_paren_is_invalid);
    RUN_TEST(test_garbage_is_invalid);
    return UNITY_END();
}
