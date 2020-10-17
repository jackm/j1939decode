#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <algorithm>
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

/* std::unordered_map object for all PGNs */
std::unordered_map<std::string, PGNData> j1939db_pgns;
/* std::unordered_map object for all SPNs */
std::unordered_map<std::string, SPNData> j1939db_spns;
/* std::unordered_map object for all source addresses */
std::unordered_map<std::string, std::string> j1939db_source_addresses;

/* Static helper functions */
static void log_msg(const char * fmt, ...);
static char * file_read(const char * filename, const char * mode);
static cJSON * create_byte_array(const uint64_t * data);
void extract_spn_data(uint32_t spn, const uint64_t * data, uint16_t start_bit, SPNData& spn_data_out);
cJSON * convert_spndata_to_cjson(const SPNData& spn_data);
void get_sa_name(uint8_t sa, std::string& name);
void get_pgn_name(uint32_t pgn, std::string& name);

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

/**************************************************************************//**

  \brief Split 64 bit number into JSON array object of bytes

  \param data     pointer to data (8 bytes total)

  \return cJSON * pointer to the JSON array object

******************************************************************************/
cJSON * create_byte_array(const uint64_t * data)
{
    cJSON * array = cJSON_CreateArray();
    if (array == NULL)
    {
        goto cleanup;
    }

    for (uint32_t i = 0; i < sizeof(data); i++)
    {
        /* Cast to uint8 pointer and then index */
        cJSON * num = cJSON_CreateNumber(((uint8_t *) data)[i]);
        if (num == NULL)
        {
            goto cleanup;
        }
        cJSON_AddItemToArray(array, num);
    }

    return array;

    cleanup:
    cJSON_Delete(array);
    return NULL;
}

/**************************************************************************//**

  \brief Extract suspect parameter number data items and decode SPN value

  \param spn           suspect parameter number
  \param data          pointer to data (8 bytes total)
  \param start_bit     starting bit of SPN in PGN (zero-order)
  \param spn_data_out  SPNData object reference

  \return void

******************************************************************************/
void extract_spn_data(uint32_t spn, const uint64_t * data, uint16_t start_bit, SPNData& spn_data_out)
{
    /* SPNData object for specific SPN data */
    SPNData spn_data;

    /* Iterator to the desired SPN data */
    auto spn_itr = j1939db_spns.find(std::to_string(spn));

    /* TODO: Use PascalCase or snake_case for JSON key names?
     * Existing J1939 lookup table uses PascalCase but snake_case may be more appropriate */

    if (spn_itr != j1939db_spns.end())
    {
        /* SPN number found in lookup table */

        /* Get the SPNData object */
        spn_data = spn_itr->second;

        /* Not currently using name or units */
        /* TODO: Support bit decodings for when the units are "Bits" */
        /* TODO: Support decoding of ASCII values when resolution is "ASCII" */

        /* Decode the data for this SPN */
        uint64_t mask = (1U << (unsigned int) spn_data.spn_length) - 1;
        uint64_t value_raw = ((*data) >> start_bit) & mask;
        double value = value_raw * spn_data.resolution + spn_data.offset;

        /* Add extra data fields */
        spn_data.start_bit = start_bit;
        spn_data.value_raw = value_raw;

        /* Check that decoded value is within operational range */
        if (value >= spn_data.operational_low && value <= spn_data.operational_high)
        {
            spn_data.value_decoded = value;
            spn_data.is_valid = true;
        }
    }
    else
    {
        log_msg("No SPN data found in database for SPN %d", spn);
        return;
    }

    spn_data_out = spn_data;
}

/**************************************************************************//**

  \brief Convert a SPNData object to a cJSON object

  \param spn_out  SPNData object reference

  \return cJSON * pointer to the SPN data JSON object

******************************************************************************/
cJSON * convert_spndata_to_cjson(const SPNData& spn_data)
{
    /* JSON object containing individual decoded SPN data */
    cJSON * spn_data_object = cJSON_CreateObject();

    if (spn_data_object == NULL)
    {
        goto end;
    }

    if (cJSON_AddStringToObject(spn_data_object, "Name", spn_data.name.c_str()) == NULL)
    {
        goto end;
    }

    if (cJSON_AddStringToObject(spn_data_object, "DataRange", spn_data.data_range.c_str()) == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(spn_data_object, "Offset", spn_data.offset) == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(spn_data_object, "OperationalHigh", spn_data.operational_high) == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(spn_data_object, "OperationalLow", spn_data.operational_low) == NULL)
    {
        goto end;
    }

    if (cJSON_AddStringToObject(spn_data_object, "OperationalRange", spn_data.operational_range.c_str()) == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(spn_data_object, "Resolution", spn_data.resolution) == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(spn_data_object, "SPNLength", spn_data.spn_length) == NULL)
    {
        goto end;
    }

    if (cJSON_AddStringToObject(spn_data_object, "Units", spn_data.units.c_str()) == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(spn_data_object, "StartBit", spn_data.start_bit) == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(spn_data_object, "ValueRaw", spn_data.value_raw) == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(spn_data_object, "ValueDecoded", spn_data.value_decoded) == NULL)
    {
        goto end;
    }

    if (cJSON_AddBoolToObject(spn_data_object, "Valid", spn_data.is_valid) == NULL)
    {
        goto end;
    }

end:
    cJSON_Delete(spn_data_object);

    /* Do not return the freed cJSON pointer to avoid use-after-free flaw
     * cJSON_Delete() does not set the freed pointer to NULL */
    return NULL;
}

/**************************************************************************//**

  \brief Get source address name

  \param sa    source address number
  \param name  std::string object reference

  \return void

******************************************************************************/
void get_sa_name(uint8_t sa, std::string& name)
{
    /* Preferred Addresses are in the range of 0 to 127 and 248 to 255 */
    if (sa <= 127 || sa >= 248)
    {
        /* Source addresses 92 through to 127 have not yet been assigned */
        if (sa >= 92 && sa <= 127)
        {
            name = std::string("Reserved");
        }
        else
        {
            /* String large enough to fit a three-digit number (8 bits) */
            auto sa_name = j1939db_source_addresses[std::to_string(sa)];
            if (sa_name.empty())
            {
                name = "Unknown";
                log_msg("No source address name found in database for source address %d", sa);
            }
        }
    }
    /* Industry Group specific addresses are in the range of 128 to 247 */
    else if (sa >= 128 && sa <= 247)
    {
        name = std::string("Industry Group specific");
    }
    else
    {
        name = std::string("Unknown");
        log_msg("Unknown source address %d outside of expected range", sa);
    }
}

/**************************************************************************//**

  \brief Get parameter group number name

  \param pgn   parameter group number
  \param name  std::string object reference

  \return void

******************************************************************************/
void get_pgn_name(uint32_t pgn, std::string& name)
{
    auto pgn_name = j1939db_pgns[std::to_string(pgn)].name;
    if (pgn_name.empty())
    {
        name = "Unknown";
        log_msg("No PGN name found in database for PGN %d", pgn);

        return;
    }

    name = pgn_name;
}

/**************************************************************************//**

  \brief Build JSON string for j1939 decoded data

  \param id         CAN identifier
  \param dlc        data length code
  \param data       pointer to data (8 bytes total)
  \param pretty     pretty print returned JSON string

  \return char *    pointer to the JSON string

******************************************************************************/
char * j1939decode_to_json(uint32_t id, uint8_t dlc, const uint64_t * data, bool pretty)
{
    /* Fail and return NULL if maps haven't been populated
     * Remember to call j1939decode_init() first! */
    if (j1939db_pgns.empty() || j1939db_spns.empty() || j1939db_source_addresses.empty())
    {
        log_msg("J1939 database not loaded");
        return NULL;
    }

    if (dlc > 8)
    {
        log_msg("DLC cannot be greater than 8 bytes");
        return NULL;
    }

    /* PGN & SA */
    const auto pgn = get_pgn(id);
    const auto sa = get_sa(id);

    /* PGNData object for specific PGN data */
    PGNData pgn_data;

    /* Iterator to the desired PGN data */
    auto pgn_itr = j1939db_pgns.find(std::to_string(pgn));

    /* Decoded flag default to false until set otherwise */
    bool decoded_flag = false;

    /* JSON string to be returned */
    char * json_string = NULL;

    /* Intermediate std::string object */
    std::string str{};

    /* JSON object containing decoded J1939 data */
    cJSON * json_object = cJSON_CreateObject();

    if (json_object == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(json_object, "ID", id) == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(json_object, "Priority", get_pri(id)) == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(json_object, "PGN", pgn) == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(json_object, "SA", sa) == NULL)
    {
        goto end;
    }

    get_sa_name(sa, str);
    if (cJSON_AddStringToObject(json_object, "SAName", str.c_str()) == NULL)
    {
        goto end;
    }

    if (cJSON_AddNumberToObject(json_object, "DLC", dlc) == NULL)
    {
        goto end;
    }

    /* Add raw data bytes to JSON object */
    cJSON_AddItemToObject(json_object, "DataRaw", create_byte_array(data));

    if (pgn_itr != j1939db_pgns.end())
    {
        /* PGN number found in lookup table */

        /* Get the PGNData object */
        pgn_data = pgn_itr->second;

        get_pgn_name(pgn, str);
        if (cJSON_AddStringToObject(json_object, "PGNName", str.c_str()) == NULL)
        {
            goto end;
        }

        /* JSON object containing list of decoded SPN data */
        cJSON * spn_object = cJSON_CreateObject();
        if (spn_object == NULL)
        {
            goto end;
        }

        if (!pgn_data.spns.empty())
        {
            /* One or more SPNs exist for PGN */

            for (auto i = 0; i < pgn_data.spns.size(); i++)
            {
                /* Getting SPN number from SPN list in PGN since the SPN number is used as a key only and not included in the SPN data object itself */
                auto spn_number = pgn_data.spns[i];

                /* Array of all possible proprietary SPNs */
                const std::vector<uint32_t> proprietary_spns{ 2550, 2551, 3328 };

                /* Check for proprietary SPNs */
                if (std::find(proprietary_spns.begin(), proprietary_spns.end(), spn_number) != proprietary_spns.end())
                {
                    /* TODO: Disabling print to silently ignore proprietary SPNs */
                    /* log_msg("Skipping decode for proprietary SPN %d", spn_number); */
                }
                else
                {
                    /* Need to pass SPN starting bit position since it is found in the PGN data object */
                    auto start_bit = pgn_data.spn_start_bits[i];
                    if (start_bit < 0)
                    {
                        log_msg("Start bit cannot be negative for SPN %d, skipping decode", spn_number);
                        continue;
                    }

                    /* SPNData object for specific SPN data */
                    SPNData spn_data;

                    extract_spn_data(spn_number, data, start_bit, spn_data);

                    if (!spn_data.name.empty())
                    {
                        /* At least one SPN found in database and actually decoded */

                        /* TODO: What criteria should be used to determine if the message should be flagged as "decoded" or not?
                            * 1. If PGN data found in database?
                            * 2. If at least one SPN found in database for PGN?
                            * 3. If at least one SPN, with start bits, found in database for PGN?
                            * 4. If at least one SPN actually decoded? (i.e. extract_spn_data() did not return NULL)
                            * Using #4 criteria for now */

                        decoded_flag = true;
                    }

                    cJSON_AddItemToObject(spn_object, std::to_string(spn_number).c_str(), convert_spndata_to_cjson(spn_data));
                }
            }
        }
        else
        {
            log_msg("No SPNs found in database for PGN %d", get_pgn(id));
        }

        /* Add SPN list object to the main JSON object */
        cJSON_AddItemToObject(json_object, "SPNs", spn_object);
    }
    else
    {
        /* TODO: This print may happen too often when trying to decode non-J1939 data */
        /* log_msg("PGN %d not found in database", get_pgn(id)); */
    }

    if (cJSON_AddBoolToObject(json_object, "Decoded", decoded_flag) == NULL)
    {
        goto end;
    }

    /* Print the JSON string
     * Memory will be allocated so remember to free it when you are done with it! */
    json_string = pretty ? cJSON_Print(json_object) : cJSON_PrintUnformatted(json_object);
    if (json_string == NULL)
    {
        log_msg("Failed to print JSON string");
    }

end:
    cJSON_Delete(json_object);
    return json_string;
}
