#include <Arduino.h>
#include <EEPROM.h>
#include "eeprom.h"
#include "global.h"

/*
    SL, StickLock
    provides an electronic lock with USB security tokens as keys.

    Copyright (C) 2019  richard.prinz@min.at

    COMMERCIAL USAGE PROHIBITED!

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program (see file gpl-3.0.txt).
    If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef CONFIG

#define CONFIG_DATA
#include "config.h"

void printDashLine() {
    for(int i=0; i<80; i++)
        Serial.print("-");
    Serial.println();
}

void printDeviceTableHeader() {
    Serial.println(F("No. Len VID  PID  Name"));
    printDashLine();
}

void printDevice(Device_t *device, uint16_t count, uint16_t len) {
    const int bufLen = 100;
    char buf[bufLen];

    snprintf(buf, bufLen, "%2d: %3d %04x %04x %s",
        count, len, device->vid, device->pid, device->name);
    Serial.println(buf);
}

void printKeyTableHeader() {
    Serial.println(F("No. Len  State (hex)"));
    Serial.println(F("    Len  Serial (hex)"));
    Serial.println(F("    Len  Key (hex)"));
    Serial.println(F("     CT               Counter"));
    printDashLine();
}

void printKey(const Key_t *key, uint16_t count, uint16_t len) {
    const uint8_t bufLen = 64;
    char buf[bufLen];
    const uint8_t bufLen2 = 32;
    char buf2[bufLen2];

    snprintf(buf, bufLen, "%2d: %3d  %02x   ",
        count, len, key->state);
    Serial.println(buf);

    snprintf(buf, bufLen, "    %3d  ",
        key->serial_len);
    Serial.print(buf);
    if(key->serial_len > 0) {
        for(int i=0; i<key->serial_len; i++) {
            snprintf(buf, bufLen, "%02x ", key->serial_bytes[i]);
            Serial.print(buf);
        }
        Serial.println();
    }
    else {
        Serial.println(F("no serial"));
    }


    snprintf(buf, bufLen, "    %3d  ",
        key->key_len);
    Serial.print(buf);
    uint8_t ip = 0;
    for(uint8_t i=0; i<key->key_len; i++) {
        ip = i + 1;
        snprintf(buf, bufLen, "%02x ", key->key_bytes[i]);
        Serial.print(buf);
        if((ip % 20) == 0 && ip < key->key_len)
            Serial.print(F("\r\n         "));
    }
    Serial.println();


    // %llu does not work on arduino so print unsigned long long counter
    // in two parts ...
    unsigned long h = (unsigned long)(key->counter/10000000000ULL);
    if(h > 0)
        snprintf(buf2, bufLen2, "%lu%lu", h,
            (unsigned long)(key->counter%10000000000ULL));
    else
        snprintf(buf2, bufLen2, "%lu",
            (unsigned long)(key->counter%10000000000ULL));
    snprintf(buf, bufLen, "    %3d  %20s", key->counter_tolerance, buf2);
    Serial.println(buf);
}

void InitEEPROM() {
    uint16_t addr;
    uint8_t deviceCount, keyCount;

    // Wipe EEPROM completely.
    ClearEEPROM();

    // Initialize device start EEPROM pointer.
    addr = EEPROM_KEY_START + sizeof(uint16_t);
    WriteDeviceStart(addr);
    addr += sizeof(uint8_t);

    // Write configured supported devices to EEPROM.
    Serial.println(F("Write the following supported devices to EEPROM:\r\n"));
    addr += PrintWriteConfigDevices(false, true, addr, &deviceCount);
    Serial.println();

    // Write number of devices to EEPROM.
    WriteDeviceCount(deviceCount);

    // Initialize key pointer in EEPROM to point after devices.
    WriteKeyStart(addr);
    addr += sizeof(uint8_t);

    // Write configured keys to EEPROM.
    Serial.println(F("Write the following keys to EEPROM:\r\n"));
    addr += PrintWriteConfigKeys(false, true, addr, &keyCount);
    Serial.println();

    // Write number of keys to EEPROM.
    WriteKeyCount(keyCount);


    /*
#ifdef DEBUG
    // Dump EEPROM for debugging.
    DumpEEPROM(0, EEPROM.length());
    Serial.println();
#endif

    Serial.println("Read devices from EEPROM for verification:\r\n");
    PrintDevices();

    Serial.println("Read keys from EEPROM for verification:\r\n");
    PrintKeys();
    */
}

void PrintDevices() {
    printDeviceTableHeader();
    Device_t device;
    uint8_t deviceCount = ReadDeviceCount();
    uint16_t addr = ReadDeviceStart();
    addr++;
    for(int d=0; d<deviceCount; d++) {
        uint16_t deviceLen = ReadDevice(addr, &device);
        printDevice(&device, d+1, deviceLen);
        InitDevice(&device);
        addr += deviceLen;
    }
}

void PrintKeys() {
    printKeyTableHeader();
    Key_t key;
    uint8_t keyCount = ReadKeyCount();
    uint16_t addr = ReadKeyStart();
    addr++;
    for(int k=0; k<keyCount; k++) {
        uint16_t keyLen = ReadKey(addr, &key);
        printKey(&key, k+1, keyLen);
        InitKey(&key);
        addr += keyLen;
    }
}

// PrintWriteConfigDevices reads user configured devices from program memory flash
// and write them to EEPROM
uint16_t PrintWriteConfigDevices(boolean quiet, boolean write, uint16_t addr,
        uint8_t *deviceCount) {
    uint16_t ttlLen = 0;
    Device_t device_s;
    const Device_t *device_f;
    uint8_t dc = 0;

    if(!quiet)
        printDeviceTableHeader();

    for(uint8_t d=0; d<sizeof(config_devices)/sizeof(config_devices[0]); d++) {
        // copy configured device from program memory flash to sram
        device_f = &config_devices[d];
        memcpy_P(&device_s, device_f, sizeof(Device_t));

        // copy device name from flash to sram
        uint16_t nl = strlen_P((const char *)pgm_read_ptr(&device_f->name));
        device_s.name = (char *)calloc(nl+1, 1);
        memcpy_P(device_s.name, pgm_read_ptr(&device_f->name), nl);

        dc = d + 1;
        uint16_t deviceLen = WriteDevice(addr, &device_s, !write);
        ttlLen += deviceLen;
        if(!quiet)
            printDevice(&device_s, dc, deviceLen);
        InitDevice(&device_s);
        if(write)
            addr += deviceLen;
    }

    if(deviceCount != NULL)
        *deviceCount = dc;
    return ttlLen;
}


// PrintWriteConfigKeys reads user configured keys from program memory flash
// and write them to EEPROM
uint16_t PrintWriteConfigKeys(boolean quiet, boolean write, uint16_t addr,
        uint8_t *keyCount) {
    uint16_t ttlLen = 0;
    Key_t key_s;
    const Key_t *key_f;
    uint8_t kc = 0;

    if(!quiet)
        printKeyTableHeader();

    for(int k=0; k<sizeof(config_keys)/sizeof(config_keys[0]); k++) {
        // copy configured key from program memory flash to sram
        key_f = &config_keys[k];
        memcpy_P(&key_s, key_f, sizeof(Key_t));

        // allocate space for serial-number bytes and copy
        // serial-number bytes from flash to sram
        if(key_s.serial_len > 0) {
            key_s.serial_bytes = (uint8_t *)malloc(key_s.serial_len);
            memcpy_P(key_s.serial_bytes, pgm_read_ptr(&key_f->serial_bytes),
                key_s.serial_len);
        }
        else
            key_s.serial_bytes = NULL;

        // allocate space for key bytes and copy key bytes
        // from flash to sram
        if(key_s.key_len > 0) {
            key_s.key_bytes = (uint8_t *)malloc(key_s.key_len);
            memcpy_P(key_s.key_bytes, pgm_read_ptr(&key_f->key_bytes),
                key_s.key_len);
        }
        else
            key_s.key_bytes = NULL;

        uint8_t kt = key_s.state & KF_KEY_TYPE;
        if(kt == KFT_HMAC_OTP_LEN_6 || kt == KFT_HMAC_OTP_LEN_8) {
            if(key_s.key_len != HMAC_KEY_LEN) {
                if(!quiet) {
                    const uint8_t bufLen = 255;
                    char buf[bufLen];

                    snprintf(buf, bufLen, "--: invalid length for HOTP key."
                        "Must be (%d) but is (%d) bytes - IGNORED",
                            HMAC_KEY_LEN, key_s.key_len);
                    Serial.println(buf);
                    InitKey(&key_s);
                    continue;
                }
            }
        }

        kc = k + 1;
        uint16_t keyLen = WriteKey(addr, &key_s, !write);
        ttlLen += keyLen;
        if(!quiet)
            printKey(&key_s, kc, keyLen);
        InitKey(&key_s);
        if(write)
            addr += keyLen;
    }

    if(keyCount != NULL)
        *keyCount = kc;
    return ttlLen;
}


// ExportDevicesAndKeys exports device and key information in EEPROM
// as C source code for copy and paste into config.h
void ExportDevicesAndKeys() {
    static const uint8_t buf_len = 200;
    char buf[buf_len];
    uint16_t addr;

    Serial.println("// ---------- CONFIG SECTION STARTS HERE ----------\r\n");

    // Write C data for devices
    Device_t device;
    uint8_t deviceCount = ReadDeviceCount();
    addr = ReadDeviceStart();
    addr++;
    Serial.println(F("// list of supported devices"));
    for(int d=0; d<deviceCount; d++) {
        uint16_t deviceLen = ReadDevice(addr, &device);
        snprintf(buf, buf_len,
            "const PROGMEM char device%d_name[] = \"%s\";",
            d+1, device.name);
        Serial.println(buf);
        InitDevice(&device);
        addr += deviceLen;
    }
    addr = ReadDeviceStart();
    addr++;
    Serial.println(F("const Device_t config_devices[] PROGMEM = {"));
    Serial.println(F("    // VID    PID     Description"));
    for(int d=0, di=1; d<deviceCount; d++, di++) {
        uint16_t deviceLen = ReadDevice(addr, &device);
        snprintf(buf, buf_len,
            "    { 0x%04x, 0x%04x, device%d_name }%c",
            device.vid, device.pid, d+1, (di==deviceCount ? '\0' : ','));
        Serial.println(buf);
        InitDevice(&device);
        addr += deviceLen;
    }
    Serial.println(F("};"));
    Serial.println("\r\n");

    // Write C data for keys
    Key_t key;
    uint8_t keyCount = ReadKeyCount();
    addr = ReadKeyStart();
    addr++;
    Serial.println(F("// define byte arrays for keys and serial numbers below"));
    for(int k=0, ki=1; k<keyCount; k++, ki++) {
        uint16_t keyLen = ReadKey(addr, &key);

        // export key length and key bytes
        snprintf(buf, buf_len,
            "static const uint8_t key%d_length = %d;",
            ki, key.key_len);
        Serial.println(buf);
        snprintf(buf, buf_len,
            "const PROGMEM uint8_t key%d[key%d_length] = {",
            ki, ki);
        Serial.print(buf);
        if(key.key_len > 0) {
            Serial.print(F("\r\n    "));
            for(uint8_t i=0, ii=1; i<key.key_len; i++, ii++) {
                snprintf(buf, buf_len, "0x%02x%s ",
                    key.key_bytes[i], (ii == key.key_len ? "\0" : ","));
                Serial.print(buf);
                if((ii % 10) == 0 && ii < key.key_len)
                    Serial.print(F("\r\n    "));
            }
        }
        Serial.println(F("};"));

        // export serial length and serial bytes
        snprintf(buf, buf_len,
            "static const uint8_t serial%d_length = %d;",
            ki, key.serial_len);
        Serial.println(buf);
        snprintf(buf, buf_len,
            "const PROGMEM uint8_t serial%d[serial%d_length] = {",
            ki, ki);
        Serial.print(buf);
        if(key.serial_len > 0) {
            Serial.print(F("\r\n    "));
            for(uint8_t i=0, ii=1; i<key.serial_len; i++, ii++) {
                snprintf(buf, buf_len, "0x%02x%s ",
                    key.serial_bytes[i], (ii == key.serial_len ? "\0" : ","));
                Serial.print(buf);
                if((ii % 10) == 0 && ii < key.serial_len)
                    Serial.print(F("\r\n    "));
            }
        }
        Serial.println(F("};\r\n\r\n"));

        InitKey(&key);
        addr += keyLen;
    }

    keyCount = ReadKeyCount();
    addr = ReadKeyStart();
    addr++;
    Serial.println(F(
        "// Note: when specifying counter values ensure they end with 'ULL'\r\n"
        "// as counters are 'unsigned long long' 64 bit values\r\n"
        "const Key_t config_keys[] PROGMEM = {"));
    for(int k=0, ki=1; k<keyCount; k++, ki++) {
        uint16_t keyLen = ReadKey(addr, &key);
        snprintf(buf, buf_len, "    // key %d\r\n    {\r\n        ", ki);
        Serial.print(buf);

        if((key.state & KF_KEY_STATE) == KFS_DISABLED)
            Serial.print(F("KFS_DISABLED | "));
        else
            Serial.print(F("KFS_ENABLED | "));

        switch(key.state & KF_KEY_TYPE) {
            case KFT_STATIC:
                Serial.print(F("KFT_STATIC"));
                break;
            case KFT_HMAC_OTP_LEN_6:
                Serial.print(F("KFT_HMAC_OTP_LEN_6"));
                break;
            case KFT_HMAC_OTP_LEN_8:
                Serial.print(F("KFT_HMAC_OTP_LEN_8"));
                break;
            default:
                Serial.print(F("KFT_RESERVED"));
                break;
        };
        snprintf(buf, buf_len, ",\r\n"
            "        serial%d_length, serial%d,\r\n"
            "        key%d_length, key%d, ",
            ki, ki, ki, ki);
        Serial.print(buf);

        // %llu does not work on arduino so print unsigned long long counter
        // in two parts ...
        if(key.counter == COUNT_ZERO)
            Serial.print(F("COUNT_ZERO, "));
        else {
            unsigned long h = (unsigned long)(key.counter/10000000000ULL);
            if(h > 0)
                snprintf(buf, buf_len, "%lu%luULL, ", h,
                    (unsigned long)(key.counter%10000000000ULL));
            else
                snprintf(buf, buf_len, "%luULL, ",
                    (unsigned long)(key.counter%10000000000ULL));
            Serial.print(buf);
        }

        if(key.counter_tolerance == DEFAULT_COUNTER_TOLERANCE)
            Serial.println("DEFAULT_COUNTER_TOLERANCE");
        else
            Serial.println(key.counter_tolerance);

        snprintf(buf, buf_len, "    }%c", (ki == keyCount ? '\0' : ','));
        Serial.println(buf);

        InitKey(&key);
        addr += keyLen;
    }
    Serial.println(F("};\r\n"));
    Serial.println(F("// ---------- CONFIG SECTION ENDS HERE ----------"));
}

#endif
