#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <Arduino.h>
#include <EEPROM.h>
#include "global.h"
#include "eeprom.h"

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

// ClearEEPROM wipes out any information in EEPROM and
// initializes all bytes to 0xFF.
void ClearEEPROM() {
    for(int i=0; i<EEPROM.length(); i++)
        EEPROM.write(i, 0xFF);
}

#ifdef CONFIG
// DumpEEPROM dumps EEPROM content in human readable form to console
// for debugging.
void DumpEEPROM(uint16_t addr, uint16_t size) {
    uint8_t c, em16, m8, m16;
    uint32_t i, e, ef;
    char buf[80], tb[20];
    memset(buf, 0, sizeof(buf));
    memset(tb, 0, sizeof(tb));

    e = addr + size;
    em16 = e % 16;
    ef = (em16 == 0) ? e : e + (16 - em16);

    for(i=addr; i<ef; i++) {
        m8 = i % 8; m16 = i % 16;
        if(i < e) {
            if(m16 == 0) {
                if(strlen(buf) > 0) {
                    sprintf(buf + strlen(buf), "  %s", tb);
                    Serial.println(buf);
                }
                sprintf(buf, "%04x:  ", i);
            }
            else if(m8 == 0)
                sprintf(buf + strlen(buf), "- ");
            c = EEPROM.read(i);
            sprintf(buf + strlen(buf), "%02x ", c);
            tb[m16] = (c > 31 && c < 127) ? c : '.';
        }
        else {
            if(m8 == 0)
                sprintf(buf + strlen(buf), "- ");
            sprintf(buf + strlen(buf), "   ");
            tb[m16] = ' ';
        }
        tb[m16+1] = 0;
    }
    sprintf(buf + strlen(buf), "  %s", tb);
    Serial.println(buf);
}
#endif

// Write16 write a 16 bit value to EEPROM.
uint16_t Write16(uint16_t addr, uint16_t value) {
    EEPROM.update(addr++, (value & 0xff00) >> 8);
    EEPROM.update(addr, value & 0x00ff);
    return sizeof(uint16_t);
}

// Read16 read a 16 bit value from EEPROM.
uint16_t Read16(uint16_t addr, uint16_t *value) {
    uint16_t h = EEPROM.read(addr++);
    uint16_t l = EEPROM.read(addr);
    *value = (h * 256) + l;
    return sizeof(uint16_t);
}

// InitDevice initializes a device struct to empty state.
void InitDevice(Device_t *device) {
    if(device == NULL)
        return;
    if(device->name != NULL) {
        free(device->name);
        device->name = NULL;
    }
    *device = {};
}

// WriteDeviceStart writes/updates EEPROM address where device entries start.
uint16_t WriteDeviceStart(uint16_t startAddr) {
    Write16(EEPROM_DEVICE_START, startAddr);
}

// ReadDeviceStart retuns address in EEPROM where device entries start.
uint16_t ReadDeviceStart() {
    uint16_t addr;
    Read16(EEPROM_DEVICE_START, &addr);
    return addr;
}

// WriteDeviceCount writes/updates number of available device entries in EEPROM.
void WriteDeviceCount(uint8_t deviceCount) {
    EEPROM.update(ReadDeviceStart(), deviceCount);
}

// ReadDeviceCount reads number of available devices entries from EEPROM.
uint8_t ReadDeviceCount() {
    return EEPROM.read(ReadDeviceStart());
}

// WriteDevice writes device entry from pointer to device struct
// to EEPROM address.
// Returns number of bytes written, 0 in case of error.
uint16_t WriteDevice(uint16_t addr, Device_t *device, boolean calcOnly) {
    if(device == NULL)
        return 0;

    // Calculate overall length in bytes for this device entry
    // and write it to EEPROM.
    uint16_t nameLen = strlen(device->name);
    uint16_t deviceLen = (sizeof(uint16_t) * 3) + sizeof(uint8_t) + nameLen;
    if(calcOnly)
        return deviceLen;

    // Wite total device entry size (2).
    addr += Write16(addr, deviceLen);

    // Write USB Vendor ID (VID) to EEPROM (2).
    addr += Write16(addr, device->vid);

    // Write USB Product ID (PID) to EEPROM (2).
    addr += Write16(addr, device->pid);

    // Write length of device description to EEPROM (1).
    EEPROM.update(addr++, nameLen);

    // Write device description to EEPROM (nameLen).
    for(int i=0; i<nameLen; i++)
        EEPROM.update(addr++, (uint8_t)(device->name)[i]);

    return deviceLen;
}

// ReadDevice read a device entry from given EEPROM address
// into device struct provided by pointer.
// Returns number of bytes read, 0 in case of error.
uint16_t ReadDevice(uint16_t addr, Device_t *device) {
    if(device == NULL)
        return 0;

    // Read overall size in bytes for this device entry from EEPROM.
    uint16_t deviceLen;
    addr += Read16(addr, &deviceLen);

    // Read USB Vendor ID (VID) from EEPROM.
    addr += Read16(addr, &device->vid);

    // Read USB Product ID (PID) from EEPROM.
    addr += Read16(addr, &device->pid);

    // Read length of device description from EEPROM.
    uint8_t nameLen = EEPROM.read(addr++);

    // Read device description from EEPROM.
    device->name = (char *)malloc(nameLen+1);
    if(device->name == NULL)
        return 0;
    memset(device->name, 0, nameLen+1);
    for(int i=0; i<nameLen; i++)
        device->name[i] = (char)EEPROM.read(addr++);
    return deviceLen;
}



// InitKey initializes a key struct to empty state.
void InitKey(Key_t *key) {
    if(key == NULL)
        return;
    if(key->serial_bytes != NULL) {
        free(key->serial_bytes);
        key->serial_bytes = NULL;
    }
    if(key->key_bytes != NULL) {
        free(key->key_bytes);
        key->key_bytes = NULL;
    }
    *key = {};
}

// WriteKeyStart writes/updates EEPROM address where key entries start.
uint16_t WriteKeyStart(uint16_t startAddr) {
    Write16(EEPROM_KEY_START, startAddr);
}

// ReadKeyStart retuns address in EEPROM where key entries start.
uint16_t ReadKeyStart() {
    uint16_t addr;
    Read16(EEPROM_KEY_START, &addr);
    return addr;
}

// WriteKeyCount writes/updates number of available key entries in EEPROM.
void WriteKeyCount(uint8_t keyCount) {
    EEPROM.update(ReadKeyStart(), keyCount);
}

// ReadKeyCount reads number of available key entries from EEPROM.
uint8_t ReadKeyCount() {
    return EEPROM.read(ReadKeyStart());
}

// WriteKey writes key entry from pointer to key struct to EEPROM address.
// Returns number of bytes written, 0 in case of error.
uint16_t WriteKey(uint16_t addr, const Key_t *key, boolean calcOnly) {
    if(key == NULL)
        return 0;

    // Calculate overall key entry size in EEPROM.
    // (See numbers in bytes in parenthes in following comments)
    uint16_t keyLen = (sizeof(uint8_t) * 4) +
        sizeof(uint16_t) + key->serial_len + key->key_len + sizeof(ULL);
    if(calcOnly)
        return keyLen;

    // Write total key size (2).
    addr += Write16(addr, keyLen);

    // Write key state (1).
    EEPROM.update(addr++, key->state);

    // Write serial length (1).
    EEPROM.update(addr++, key->serial_len);

    // Write key bytes (key->serial_len).
    for(int i=0; i<key->serial_len; i++)
        EEPROM.update(addr++, (uint8_t)(key->serial_bytes)[i]);

    // Write key length (1).
    EEPROM.update(addr++, key->key_len);

    // Write key bytes (key->key_len).
    for(int i=0; i<key->key_len; i++)
        EEPROM.update(addr++, (uint8_t)(key->key_bytes)[i]);

    // Write counter (8).
    union {
        ULL cnt;
        uint8_t bytes[sizeof(key->counter)];
    } cnt_bytes;
    cnt_bytes.cnt = key->counter;
    for(int i=0; i<sizeof(key->counter); i++)
        EEPROM.update(addr++, cnt_bytes.bytes[i]);

    // Write counter tolerance (1)
    EEPROM.update(addr++, key->counter_tolerance);

    return keyLen;
}

// ReadKey read a key entry from given EEPROM address
// into key struct provided by pointer.
// Returns number of bytes read, 0 in case of error.
uint16_t ReadKey(uint16_t addr, Key_t *key) {
    if(key == NULL)
        return 0;

    // Read overall key len in bytes for this key entry in EEPROM.
    uint16_t keyLen;
    addr += Read16(addr, &keyLen);

    // Read key state.
    key->state = EEPROM.read(addr++);

    // Read serial len.
    key->serial_len = EEPROM.read(addr++);

    // Read key bytes.
    (key->serial_bytes) = (uint8_t *)malloc(key->serial_len);
    if(key->serial_bytes == NULL)
        return 0;
    memset(key->serial_bytes, 0, key->serial_len);
    for(int i=0; i<key->serial_len; i++)
        key->serial_bytes[i] = (uint8_t)EEPROM.read(addr++);

    // Read key len.
    key->key_len = EEPROM.read(addr++);

    // Read key bytes.
    (key->key_bytes) = (uint8_t *)malloc(key->key_len);
    if(key->key_bytes == NULL)
        return 0;
    memset(key->key_bytes, 0, key->key_len);
    for(int i=0; i<key->key_len; i++)
        key->key_bytes[i] = (uint8_t)EEPROM.read(addr++);

    // Read key counter.
    union {
        ULL cnt;
        uint8_t bytes[sizeof(key->counter)];
    } cnt_bytes;
    for(int i=0; i<sizeof(key->counter); i++)
        cnt_bytes.bytes[i] = (uint8_t)EEPROM.read(addr++);
    key->counter = cnt_bytes.cnt;

    // Read counter tolerance.
    key->counter_tolerance = EEPROM.read(addr++);

    return keyLen;
}
