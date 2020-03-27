#ifndef J1939_H
#define J1939_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Project version */
#define J1939_DECODE_VERSION_MAJOR 2
#define J1939_DECODE_VERSION_MINOR 0
#define J1939_DECODE_VERSION_PATCH 0

/* J1939 digital annex JSON filename */
#define J1939DB "J1939db.json"

/* Print version string */
const char * j1939_version(void);

/* Initialize and allocate memory for J1939 lookup table */
void j1939_init(void);

/* Deinitialize and free memory for J1939 lookup table */
void j1939_deinit(void);

/* Build JSON string for j1939 decoded data
 * Memory will be allocated so remember to free the string when you are done with it! */
/* Example JSON output format:
{
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
char * j1939_decode_to_json(uint32_t id, uint8_t dlc, const uint64_t * data, bool pretty);

#ifdef __cplusplus
}
#endif

#endif //J1939_H
