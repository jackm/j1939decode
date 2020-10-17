#ifndef J1939DECODE_H
#define J1939DECODE_H

//? #ifdef __cplusplus
//? extern "C" {
//? #endif

//? #include <stdint.h>
//? #include <stdbool.h>

#include <cstdint>
#include <string>
#include <vector>

/* Project version */
#define J1939DECODE_VERSION_MAJOR 3
#define J1939DECODE_VERSION_MINOR 1
#define J1939DECODE_VERSION_PATCH 1

/* J1939 digital annex JSON filename */
#define J1939DECODE_DB "/home/nathanl/projects/palisade/attacks/J1939db.json"  //! LOCAL ONLY

/* PGN data struct */
struct PGNData
{
	std::string label{};
	std::string name{};
	std::string pgn_length{};
	std::string rate{};

	std::vector<uint32_t> spns;
	std::vector<int8_t> spn_start_bits;
};

/* SPN data struct */
struct SPNData
{
	std::string data_range{};
	std::string name{};
	std::string operational_range{};
	std::string units{};

	double offset{ 0.0 };
	double operational_high{ 0.0 };
	double operational_low{ 0.0 };
	double resolution{ 0.0 };
	uint8_t	spn_length{ 0U };
};

/* Log function pointer type */
typedef void (*log_fn_ptr)(const char *);

/* Set log function handler */
void j1939decode_set_log_fn(log_fn_ptr fn);

/* Print version string */
const char * j1939decode_version(void);

/* Initialize and allocate memory for J1939 lookup table */
void j1939decode_init(void);

/* Clears the J1939 maps */
void j1939decode_deinit(void);

/* Build JSON string for j1939 decoded data
 * Memory will be allocated so remember to free the string when you are done with it! */
/* Example JSON output format:
{
	"ID":	419348235,
	"Priority":	6,
	"PGN":	65215,
	"SA":	11,
	"SAName":	"Brakes - System Controller",
	"DLC":	8,
	"DataRaw":	[170, 15, 125, 125, 125, 125, 255, 255],
	"PGNName":	"Wheel Speed Information",
	"SPNs":	{
		"904":	{
			"DataRange":	"0 to 250.996 km/h",
			"Name":	"Front Axle Speed",
			"Offset":	0,
			"OperationalHigh":	250.996,
			"OperationalLow":	0,
			"OperationalRange":	"",
			"Resolution":	0.00390625,
			"SPNLength":	16,
			"Units":	"km/h",
			"StartBit":	0,
			"ValueRaw":	4010,
			"ValueDecoded":	15.6640625,
			"Valid":	true
		},
		...
    },
	"Decoded":	true
}
 */
char * j1939decode_to_json(uint32_t id, uint8_t dlc, const uint64_t * data, bool pretty);

//? #ifdef __cplusplus
//? }
//? #endif

#endif //J1939DECODE_H
