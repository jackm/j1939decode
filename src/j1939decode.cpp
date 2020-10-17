#include <cstdio>
//? #include <stdbool.h>
#include <cstdlib>
#include <cstdarg>
#include <unordered_map>

#include "j1939decode.h"

extern "C"
{
#include "cJSON.h"
}

/* Log function pointer */
static log_fn_ptr log_fn = NULL;

/* J1939 lookup table pointer */
static cJSON * j1939db_json = NULL;

/* JSON object for all PGNs */
//? static cJSON * j1939db_pgns = NULL;
std::unordered_map<std::string, PGNData> j1939db_pgns;
/* JSON object for all SPNs */
//? static cJSON * j1939db_spns = NULL;
std::unordered_map<std::string, SPNData> j1939db_spns;
/* JSON object for all source addresses */
//? static cJSON * j1939db_source_addresses = NULL;
std::unordered_map<std::string, std::string> j1939db_source_addresses;

/* Static helper functions */
static void log_msg(const char * fmt, ...);
static char * file_read(const char * filename, const char * mode);
static bool in_array(uint32_t val, const uint32_t * array, size_t len);
static cJSON * create_byte_array(const uint64_t * data);
static const cJSON * get_pgn_data(uint32_t pgn);
static const cJSON * get_spn_data(uint32_t spn);
static cJSON * extract_spn_data(uint32_t spn, const uint64_t * data, uint32_t start_bit);
static char * get_sa_name(uint8_t sa);
static char * get_pgn_name(uint32_t pgn);

/* Extract J1939 sub fields from CAN ID */
static inline uint8_t get_pri(uint32_t id)
{
    /* 3-bit priority */
    return (uint8_t) ((id >> (18U + 8U)) & ((1U << 3U) - 1));
}
static inline uint32_t get_pgn(uint32_t id)
{
    /* 18-bit parameter group number */
    return (uint32_t) ((id >> 8U) & ((1U << 18U) - 1));
}
static inline uint8_t get_sa(uint32_t id)
{
    /* 8-bit source address */
    return (uint8_t) ((id >> 0U) & ((1U << 8U) - 1));
}

/**************************************************************************//**

  \brief Log formatted message to user defined handler, or stderr as default

  \return void

******************************************************************************/
void log_msg(const char * fmt, ...)
{
    char buf[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (log_fn)
    {
        (*log_fn)(buf);
    }
    else
    {
        /* Default print to stderr */
        fprintf(stderr, "%s\n", buf);
    }
}

/**************************************************************************//**

  \brief  Read file contents into allocated memory

  \return pointer to string containing file contents

******************************************************************************/
char * file_read(const char * filename, const char * mode)
{
    FILE * fp = fopen(filename, mode);
    if (fp == NULL)
    {
        log_msg("Could not open file %s", filename);
        /* No need to close the file before returning since it was never opened */
        return NULL;
    }

    /* Get total file size */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    /* Allocate enough memory for entire file */
    char * buf = (char *)malloc(file_size + 1);
    if (buf == NULL)
    {
        log_msg("Memory allocation failure");
        fclose(fp);
        return NULL;
    }

    /* Read the entire file into a buffer */
    long read_size = fread(buf, 1, file_size, fp);
    if (read_size != file_size)
    {
        log_msg("Read %ld of %ld total bytes in file %s", read_size, file_size, filename);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    // Null terminator
    buf[file_size] = '\0';

    return buf;
}

/**************************************************************************//**

  \brief Set log function handler

  \return void

******************************************************************************/
void j1939decode_set_log_fn(log_fn_ptr fn)
{
    if (fn)
    {
        log_fn = fn;
    }
}

/**************************************************************************//**

  \brief Print version string

  \return version string

******************************************************************************/
const char * j1939decode_version(void)
{
    static char version[15];
    sprintf(version, "%i.%i.%i", J1939DECODE_VERSION_MAJOR, J1939DECODE_VERSION_MINOR, J1939DECODE_VERSION_PATCH);

    return version;
}

/**************************************************************************//**

  \brief Initialize and allocate memory for J1939 lookup table

  \return void

******************************************************************************/
void j1939decode_init(void)
{
    char * s;
    /* Memory-map all file contents */
    s = file_read(J1939DECODE_DB, "r");
    if (s == NULL)
    {
        return;
    }

    j1939db_json = cJSON_Parse(s);
    free(s);
    if (j1939db_json == NULL)
    {
        log_msg("Unable to parse J1939db");
        return;
    }

    /* Parse JSON objects into maps */
    auto pgn_root = cJSON_GetObjectItemCaseSensitive(j1939db_json, "J1939PGNdb");
    auto spn_root = cJSON_GetObjectItemCaseSensitive(j1939db_json, "J1939SPNdb");
    auto sa_root = cJSON_GetObjectItemCaseSensitive(j1939db_json, "J1939SATabledb");

    auto pgn_item = pgn_root->child ? pgn_root->child : nullptr;
    auto pgn_data = PGNData();

    while (pgn_item)
    {
        if (std::string(pgn_item->string) == "131071")
        {
            int i = 5;
        }
        pgn_data.label = std::string(cJSON_GetObjectItemCaseSensitive(pgn_item, "Label")->valuestring);
        pgn_data.name = std::string(cJSON_GetObjectItemCaseSensitive(pgn_item, "Name")->valuestring);
        pgn_data.pgn_length = std::string(cJSON_GetObjectItemCaseSensitive(pgn_item, "PGNLength")->valuestring);
        pgn_data.rate = std::string(cJSON_GetObjectItemCaseSensitive(pgn_item, "Rate")->valuestring);

        auto spns_array = cJSON_GetObjectItemCaseSensitive(pgn_item, "SPNs");
        auto spns_size = cJSON_GetArraySize(spns_array);

        for (auto i = 0; i < spns_size; ++i)
        {
            pgn_data.spns.emplace_back(
                static_cast<uint32_t>(cJSON_GetArrayItem(spns_array, i)->valueint)
            );
        }

        auto spn_bits_array = cJSON_GetObjectItemCaseSensitive(pgn_item, "SPNStartBits");
        auto spn_bits_size = cJSON_GetArraySize(spn_bits_array);

        for (auto i = 0; i < spn_bits_size; ++i)
        {
            pgn_data.spn_start_bits.emplace_back(
                static_cast<int8_t>(cJSON_GetArrayItem(spn_bits_array, i)->valueint)
            );
        }

        j1939db_pgns.insert({ std::string(pgn_item->string), pgn_data });
        pgn_data.spns.clear();
        pgn_data.spn_start_bits.clear();

        pgn_item = pgn_item->next;
    }

    auto spn_item = spn_root->child ? spn_root->child : nullptr;
    auto spn_data = SPNData();

    while (spn_item)
    {
        spn_data.data_range = std::string(cJSON_GetObjectItemCaseSensitive(spn_item, "DataRange")->valuestring);
        spn_data.name = std::string(cJSON_GetObjectItemCaseSensitive(spn_item, "Name")->valuestring);
        spn_data.offset = cJSON_GetObjectItemCaseSensitive(spn_item, "Offset")->valuedouble;
        spn_data.operational_high = cJSON_GetObjectItemCaseSensitive(spn_item, "OperationalHigh")->valuedouble;
        spn_data.operational_low = cJSON_GetObjectItemCaseSensitive(spn_item, "OperationalLow")->valuedouble;
        spn_data.operational_range = std::string(cJSON_GetObjectItemCaseSensitive(spn_item, "OperationalRange")->valuestring);
        spn_data.resolution = cJSON_GetObjectItemCaseSensitive(spn_item, "Resolution")->valuedouble;
        spn_data.spn_length = static_cast<uint8_t>(cJSON_GetObjectItemCaseSensitive(spn_item, "SPNLength")->valueint);
        spn_data.units = std::string(cJSON_GetObjectItemCaseSensitive(spn_item, "Units")->valuestring);

        j1939db_spns.insert({ std::string(spn_item->string), spn_data });
        spn_item = spn_item->next;
    }

    auto sa_item = sa_root->child ? sa_root->child : nullptr;

    while (sa_item)
    {
        j1939db_source_addresses.insert({
                std::string(sa_item->string),
                std::string(sa_item->valuestring)
            }
        );
        sa_item = sa_item->next;
    }

    /* cJSON_Delete() checks if pointer is NULL before freeing */
    cJSON_Delete(j1939db_json);

    /* Explicitly set pointer to NULL */
    j1939db_json = nullptr;
}

/**************************************************************************//**

  \brief Clears the J1939 maps

  \return void

******************************************************************************/
void j1939decode_deinit(void)
{
    j1939db_pgns.clear();
    j1939db_spns.clear();
    j1939db_source_addresses.clear();
}

int main()
{
    j1939decode_init();

    return 0;
}
