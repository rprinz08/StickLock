#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef CONFIG_DATA

/*
 * To configure  HMAC OTP Keys and supported devices uncomment the
 * "#define CONFIG" line in file "global.h" and modify key definition
 * bytes for keys and devices below. Compile and flash sketch to
 * board an run it.
 * Then comment "//#define CONFIG" line and flash board again.
 * Now the board works in lock mode.
 */


// The following section contains some sample keys.
// Remove/modify to suit your needs/keys !!

// ---------- CONFIG SECTION STARTS HERE ----------

// list of supported devices
const PROGMEM char device1_name[] = "YubiKey NEO NFC";
const PROGMEM char device2_name[] = "YubiKey 5 NFC";
const PROGMEM char device3_name[] = "Dell Keyboard";
const Device_t config_devices[] PROGMEM = {
    // VID         PID                  Description
    { YUBIKEY_VID, YUBIKEY_NEO_PID,     device1_name },
    { YUBIKEY_VID, YUBIKEY_5_NFC_PID,   device2_name },
    { 0x413c,      0x2106,              device3_name }
};


// define byte arrays for keys and serial numbers below
// YubiKey HOTP-8
static const uint8_t key1_length = 20;
const PROGMEM uint8_t key1[key1_length] = {
    0x07, 0x29, 0xd3, 0xee, 0x63, 0x28, 0x5f, 0x71, 0x4a, 0xd8,
    0xcf, 0x74, 0xc7, 0x70, 0xda, 0x8a, 0x88, 0x8b, 0x62, 0xcf };
static const uint8_t serial1_length = 20;
const PROGMEM uint8_t serial1[serial1_length] = { // 000 4870111
    0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x34, 0x00, 0x38, 0x00,
    0x37, 0x00, 0x30, 0x00, 0x31, 0x00, 0x31, 0x00, 0x31, 0x00 };


// YubiKey static 64 char
static const uint8_t key2_length = 64;
const PROGMEM uint8_t key2[key2_length] = {
    0x31, 0x4e, 0x56, 0x32, 0x62, 0x68, 0x75, 0x66, 0x6e, 0x76,
    0x75, 0x72, 0x64, 0x72, 0x6e, 0x67, 0x6b, 0x62, 0x6a, 0x6c,
    0x68, 0x72, 0x68, 0x74, 0x64, 0x6c, 0x62, 0x62, 0x6e, 0x62,
    0x6b, 0x74, 0x76, 0x69, 0x6a, 0x64, 0x63, 0x76, 0x64, 0x67,
    0x68, 0x74, 0x6c, 0x64, 0x68, 0x6b, 0x62, 0x68, 0x75, 0x67,
    0x6e, 0x6b, 0x72, 0x63, 0x76, 0x6a, 0x67, 0x76, 0x69, 0x62,
    0x67, 0x74, 0x6a, 0x74 };
static const uint8_t serial2_length = 0;
const PROGMEM uint8_t serial2[serial2_length] = {};


// YubiKey HOTP-6
static const uint8_t key3_length = 20;
const PROGMEM uint8_t key3[key3_length] = {
    0x3b, 0xc1, 0xc3, 0x25, 0x98, 0x42, 0x7b, 0x19, 0x06, 0x2b,
    0x32, 0x6d, 0x54, 0x2d, 0x6b, 0xc6, 0x4a, 0x3b, 0x2c, 0x89 };
static const uint8_t serial3_length = 20;
const PROGMEM uint8_t serial3[serial3_length] = { // 000 3505222
    0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x33, 0x00, 0x35, 0x00,
    0x30, 0x00, 0x35, 0x00, 0x32, 0x00, 0x32, 0x00, 0x32, 0x00 };


// Dell Keyboard test static 4 char (key = 'test')
static const uint8_t key4_length = 4;
const PROGMEM uint8_t key4[key4_length] = {
    0x74, 0x65, 0x73, 0x74 };
static const uint8_t serial4_length = 0;
const PROGMEM uint8_t serial4[serial4_length] = {};


// YubiKey Serial Number Only
static const uint8_t key5_length = 0;
const PROGMEM uint8_t key5[key5_length] = {};
static const uint8_t serial5_length = 20;
const PROGMEM uint8_t serial2[serial5_length] = { // 000 1234567
    0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x31, 0x00, 0x32, 0x00,
    0x33, 0x00, 0x33, 0x00, 0x34, 0x00, 0x35, 0x00, 0x36, 0x00 };



// Note: when specifying counter values ensure they end with 'ULL'
// as counters are 'unsigned long long' 64 bit values
const Key_t config_keys[] PROGMEM = {
    // key 1
    { KFS_ENABLED | KFT_HMAC_OTP_LEN_8,
        serial1_length, serial1,
        key1_length, key1, 20ULL, DEFAULT_COUNTER_TOLERANCE
    },
    // key 2
    { KFS_ENABLED | KFT_STATIC,
        serial2_length, serial2,
        key2_length, key2, COUNT_ZERO, ZERO_COUNTER_TOLERANCE
    },
    // key 3
    { KFS_ENABLED | KFT_HMAC_OTP_LEN_6,
        serial3_length, serial3,
        key3_length, key3, COUNT_ZERO, DEFAULT_COUNTER_TOLERANCE
    },
    // key 4
    { KFS_ENABLED | KFT_STATIC,
        serial4_length, serial4,
        key4_length, key4, COUNT_ZERO, ZERO_COUNTER_TOLERANCE
    }
    // key 5
    { KFS_ENABLED | KFT_SERIAL_NUMBER,
        serial5_length, serial5,
        key4_length, key5, COUNT_ZERO, ZERO_COUNTER_TOLERANCE
    }
};

// ---------- CONFIG SECTION ENDS HERE ----------

#endif

extern void InitEEPROM();
extern void PrintDevices();
extern void PrintKeys();
extern uint16_t PrintWriteConfigDevices(boolean quiet, boolean write,
    uint16_t addr, uint8_t *deviceCount);
extern uint16_t PrintWriteConfigKeys(boolean quiet, boolean write,
    uint16_t addr, uint8_t *keyCount);
extern void ExportDevicesAndKeys();

#endif
