#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "unity.h"

#include "j1939decode.h"
#include "cJSON.h"


static uint8_t pri;
static uint32_t pgn;
static uint8_t sa;
static uint8_t dlc;
static uint8_t data[8];

/* Compile CAN ID from J1939 sub fields */
static inline uint32_t get_id(uint8_t pri, uint32_t pgn, uint8_t sa)
{
    return (uint32_t) (
        ((pri & ((1U <<  3U) - 1)) << (18U + 8U)) +
        ((pgn & ((1U << 18U) - 1)) << 8U) +
        ((sa  & ((1U <<  8U) - 1)) << 0U)
    );
}


void setUp(void)
{
    j1939decode_init();

    /* Default values */
    pri = 0;
    pgn = 0;
    sa = 0;
    dlc = 8;
    for (size_t i = 0; i < sizeof(data); i++)
    {
        data[i] = 0xFF;
    }
}

void tearDown(void)
{
    j1939decode_deinit();
}

void test_j1939decode_version_number(void)
{
    TEST_ASSERT_EQUAL_STRING("3.1.1", j1939decode_version());
}

void test_j1939decode_return_not_null(void)
{
    /* If message can be converted to JSON, j1939decode_to_json() will return non-null pointer */
    TEST_ASSERT_NOT_NULL(j1939decode_to_json(get_id(pri, pgn, sa), dlc, (uint64_t *) data, false));
}

void test_j1939decode_return_parsable_json(void)
{
    char * json_string = j1939decode_to_json(get_id(pri, pgn, sa), dlc, (uint64_t *) data, false);
    cJSON * json = cJSON_Parse(json_string);

    /* If JSON string can be parsed, cJSON_Parse() will return non-null pointer */
    TEST_ASSERT_NOT_NULL(json);

    cJSON_Delete(json);
    free(json_string);
}

void test_j1939decode_message_decoded_true(void)
{
    /* Set to any real PGN that exists in J1939 database */
    pgn = 0;

    char * json_string = j1939decode_to_json(get_id(pri, pgn, sa), dlc, (uint64_t *) data, false);
    cJSON * json = cJSON_Parse(json_string);

    char * item_string = cJSON_Print(cJSON_GetObjectItemCaseSensitive(json, "Decoded"));

    /* "Decoded" key should be true when given a real PGN */
    TEST_ASSERT_EQUAL_STRING("true", item_string);

    free(item_string);
    cJSON_Delete(json);
    free(json_string);
}

void test_j1939decode_message_decoded_false(void)
{
    /* PGN 1 does not exist in J1939 database */
    pgn = 1;

    char * json_string = j1939decode_to_json(get_id(pri, pgn, sa), dlc, (uint64_t *) data, false);
    cJSON * json = cJSON_Parse(json_string);

    char * item_string = cJSON_Print(cJSON_GetObjectItemCaseSensitive(json, "Decoded"));

    /* "Decoded" key should be false when given non-existent PGN */
    TEST_ASSERT_EQUAL_STRING("false", item_string);

    free(item_string);
    cJSON_Delete(json);
    free(json_string);
}

void test_j1939decode_message_data_raw(void)
{
    /* Set arbitrary data values */
    data[0] = 11;
    data[1] = 22;
    data[2] = 33;
    data[3] = 44;
    data[4] = 55;
    data[5] = 66;
    data[6] = 77;
    data[7] = 88;

    char * json_string = j1939decode_to_json(get_id(pri, pgn, sa), dlc, (uint64_t *) data, false);
    cJSON * json = cJSON_Parse(json_string);

    char * item_string = cJSON_Print(cJSON_GetObjectItemCaseSensitive(json, "DataRaw"));

    /* "DataRaw" key should be the same as the raw message data bytes */
    TEST_ASSERT_EQUAL_STRING("[11, 22, 33, 44, 55, 66, 77, 88]", item_string);

    free(item_string);
    cJSON_Delete(json);
    free(json_string);
}

void test_j1939decode_message_proprietary_pgn(void)
{
    /* PGN 65280 is a proprietary PGN for manufacturer defined usage */
    pgn = 65280;

    char * json_string = j1939decode_to_json(get_id(pri, pgn, sa), dlc, (uint64_t *) data, false);
    cJSON * json = cJSON_Parse(json_string);

    char * item_string = cJSON_Print(cJSON_GetObjectItemCaseSensitive(json, "Decoded"));

    /* "Decoded" key should be false when given a proprietary PGN */
    TEST_ASSERT_EQUAL_STRING("false", item_string);

    free(item_string);
    cJSON_Delete(json);
    free(json_string);
}

void test_j1939decode_message_sa_reserved(void)
{
    char * json_string;
    cJSON * json;

    for(int i = 92; i <= 127; i++)
    {
        sa = i;

        json_string = j1939decode_to_json(get_id(pri, pgn, sa), dlc, (uint64_t *) data, false);
        json = cJSON_Parse(json_string);

        /* "SAName" key should be set to "Reserved" if SA is between 92 through to 127 inclusive */
        TEST_ASSERT_EQUAL_STRING("Reserved", cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, "SAName")));
    }

    cJSON_Delete(json);
    free(json_string);
}

void test_j1939decode_message_sa_industry_group_specific(void)
{
    char * json_string;
    cJSON * json;

    for(int i = 128; i <= 247; i++)
    {
        sa = i;

        json_string = j1939decode_to_json(get_id(pri, pgn, sa), dlc, (uint64_t *) data, false);
        json = cJSON_Parse(json_string);

        /* "SAName" key should be set to "Industry Group specific" if SA is between 128 through to 247 inclusive */
        TEST_ASSERT_EQUAL_STRING("Industry Group specific", cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, "SAName")));
    }

    cJSON_Delete(json);
    free(json_string);
}
