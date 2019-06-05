#ifndef _GLOBAL_H_
#define _GLOBAL_H_


// ----------------------------------------------------------------------------

// Configuration settings

// Uncomment the following line to enable config mode.
// In this mode administrative functions like writing
// keys to EEPROM can be performed
// Note: Normal StickLock operation is disabled when
//       CONFIG is enabled
#define CONFIG


// ****************************************************************************
// WARNING: The following options should all be commented out
// for StickLock to operate correctly. They affect the memory
// footprint and are normally not required for standard
// operation. Ensure that after compiling at least 730 bytes
// are available for local variables (shown in console output).
// Otherwise you might notice wrong behaviour (e.g. long 64
// byte static keys might no longer be recognized). Below 700
// bytes no key type will work!
// ****************************************************************************

// Uncomment to enable debug mode which shows additional debug information
// Note: When enabling this, ENABLE_UI must also be enabled
//#define DEBUG

// Uncomment to enable a simple serial user interface
// Note: If DEBUG is enabled this also needs to be enabled
//#define ENABLE_UI

// Uncomment to disable supported device checks
// Note: SHOULD NOT BE DONE
//#define DISABLE_SUPPORTED_DEVICE_CHECKS

// Uncomment to disable clear switch
//#define DISABLE_CLEAR_SWITCH

// ----------------------------------------------------------------------------



#ifdef DEBUG
#define ENABLE_UI
#endif

#define _STRINGIFY(x)               #x
#define _TOSTRING(x)                _STRINGIFY(x)

#define VERSION_MA                  1
#define VERSION_MI                  0
#define VERSION_PA                  0
#define VERSION                     _TOSTRING(VERSION_MA.VERSION_MI.VERSION_PA)

#define MAX_INPUT_LEN               200
#define HMAC_KEY_LEN                20
#define DEFAULT_COUNTER_TOLERANCE   20
#define ZERO_COUNTER_TOLERANCE      0
#define END_OF_INPUT_CHAR           19

// all times in milliseconds
#define UNLOCK_TIME                 3000
#define UNLOCK_INDICATOR_TIME       3000
#define ERROR_INDICATOR_TIME        1000
#define CLEAR_SWITCH_ACCEPT         6000

#define ERROR_INDICATOR_REPEAT      10

#define EEPROM_DEVICE_START         0x0000
#define EEPROM_KEY_START            0x0002

const uint8_t CLEAR_SWITCH_PIN = 8;
const uint8_t GREEN_LED_PIN = 7;
const uint8_t RED_LED_PIN = 6;
const uint8_t UNLOCK_PIN = 5;
const uint8_t DEVICE_PIN = 4;
const uint8_t POWER_OFF_PIN = 3;

typedef unsigned long long ULL;

// Key State Flag
// specifies if key is enabled or disabled
#define KF_KEY_STATE                1 // bit mask of flag in key state byte
#define KFS_DISABLED                0 // key disabled = not usable
#define KFS_ENABLED                 1 // key enabled = usable

// Key Type Flag
// specifies type of key
#define KF_KEY_TYPE                 6 // mask of flag in key state byte
#define KFT_STATIC                  0 // static key
#define KFT_HMAC_OTP_LEN_6          2 // HMAC OTP 6 byte length
#define KFT_HMAC_OTP_LEN_8          4 // HMAC OTP 8 byte length
#define KFT_RESERVED                6 // reserved for future use

#define COUNT_ZERO                  0ULL
#define COUNT_MAX                   0xFFFFFFFFFFFFFFFFULL // == decimal 18446744073709551615ULL
#define NO_SERIAL                   {}

struct sKey {
    uint8_t state;
    uint8_t serial_len;
    union {
        const uint8_t *serial_bytes_c;
        uint8_t *serial_bytes;
    };
    uint8_t key_len;
    union {
        const uint8_t *key_bytes_c;
        uint8_t *key_bytes;
    };
    ULL counter;
    uint8_t counter_tolerance;
};
typedef struct sKey Key_t;

struct sDevice {
    uint16_t vid;
    uint16_t pid;
    union {
        const char *name_c;
        char *name;
    };
};
typedef struct sDevice Device_t;


// Some common devices.
#define YUBIKEY_VID                 0x1050
#define YUBIKEY_NEO_PID             0x0116
#define YUBIKEY_5_NFC_PID           0x0407
#endif
