#include <stdlib.h>

#include "unity.h"

#include "j1939.h"

/* Need to re-include these because Ceedling doesn't find them even though they are in j1939.c? */
#include "cJSON.h"
#include "file.h"

static j1939_header_t header;
static uint8_t data[8];

void setUp(void)
{
    j1939_init();

    /* Default header and data values */
    header = (j1939_header_t) {
            .pri = 0,
            .pgn = 0,
            .sa = 0,
            .dlc = 8,
    };
    for (int i = 0; i < sizeof(data); i++)
    {
        data[i] = 0xFF;
    }
}

void tearDown(void)
{
    j1939_deinit();
}

void test_j1939decode_version_number(void)
{
    TEST_ASSERT_EQUAL_STRING("1.0.1", j1939_version());
}

void test_j1939decode_return_not_null(void)
{
    /* If message can be converted to JSON, j1939_decode_to_json() will return non-null pointer */
    TEST_ASSERT_NOT_NULL(j1939_decode_to_json(&header, (uint64_t *) data, 0));
}

void test_j1939decode_return_parsable_json(void)
{
    char * json_string = j1939_decode_to_json(&header, (uint64_t *) data, 0);
    cJSON * json = cJSON_Parse(json_string);

    /* If JSON string can be parsed, cJSON_Parse() will return non-null pointer */
    TEST_ASSERT_NOT_NULL(json);

    cJSON_Delete(json);
    free(json_string);
}

void test_j1939decode_message_decoded_true(void)
{
    /* Set to any real PGN that exists in J1939 database */
    header.pgn = 0;

    char * json_string = j1939_decode_to_json(&header, (uint64_t *) data, 0);
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
    header.pgn = 1;

    char * json_string = j1939_decode_to_json(&header, (uint64_t *) data, 0);
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

    char * json_string = j1939_decode_to_json(&header, (uint64_t *) data, 0);
    cJSON * json = cJSON_Parse(json_string);

    char * item_string = cJSON_Print(cJSON_GetObjectItemCaseSensitive(json, "DataRaw"));

    /* "DataRaw" key should be the same as the raw message data bytes */
    TEST_ASSERT_EQUAL_STRING("[11, 22, 33, 44, 55, 66, 77, 88]", item_string);

    free(item_string);
    cJSON_Delete(json);
    free(json_string);
}

void test_j1939decode_message_sa_reserved(void)
{
    char * json_string;
    cJSON * json;

    for(int sa = 92; sa <= 127; sa++)
    {
        header.sa = sa;

        json_string = j1939_decode_to_json(&header, (uint64_t *) data, 0);
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

    for(int sa = 128; sa <= 247; sa++)
    {
        header.sa = sa;

        json_string = j1939_decode_to_json(&header, (uint64_t *) data, 0);
        json = cJSON_Parse(json_string);

        /* "SAName" key should be set to "Industry Group specific" if SA is between 128 through to 247 inclusive */
        TEST_ASSERT_EQUAL_STRING("Industry Group specific", cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json, "SAName")));
    }

    cJSON_Delete(json);
    free(json_string);
}
