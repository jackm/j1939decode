#include "unity.h"

#include "j1939.h"

/* Need to re-include these because Ceedling doesn't find them even though they are in j1939.c? */
#include "cJSON.h"
#include "file.h"

void setUp(void)
{
    j1939_init();
}

void tearDown(void)
{
    j1939_deinit();
}

void test_j1939decode_version_number(void)
{
    TEST_ASSERT_EQUAL_STRING("1.0.0", j1939_version());
}

void test_j1939decode_return_not_null(void)
{
    j1939_header_t header = {
            .pri = 6,
            .pgn = 0,
            .sa = 0,
            .dlc = 8,
    };
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    TEST_ASSERT_NOT_NULL(j1939_decode_to_json(&header, (uint64_t *) data, 0));
}
