#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

static void test_sanity() { TEST_ASSERT_EQUAL_INT(2, 1 + 1); }

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_sanity);
    return UNITY_END();
}
