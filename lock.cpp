#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <Arduino.h>
#include "global.h"
#include "led.h"
#include "eeprom.h"
#include "hmac.h"
#include "lock.h"
#ifdef ENABLE_UI
#include <usbhid.h>
#endif


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

#ifndef CONFIG

LockHandler::LockHandler(uint8_t device_pin, uint8_t unlock_pin,
        unsigned long unlock_time) {
    this->device_pin = device_pin;
    this->unlock_pin = unlock_pin;
    this->unlock_time = unlock_time;

    pinMode(this->device_pin, OUTPUT);
    pinMode(this->unlock_pin, OUTPUT);

    Reset();
}


void LockHandler::DeviceSupported() {
    digitalWrite(this->device_pin, HIGH);
#ifdef DEBUG
    Serial.println("Lock: DeviceSupported");
#endif
}
void LockHandler::DeviceSupportedReset() {
    digitalWrite(this->device_pin, LOW);
#ifdef DEBUG
    Serial.println("Lock: DeviceSupportedReset");
#endif
}


void LockHandler::KeyValid() {
    digitalWrite(this->unlock_pin, HIGH);
#ifdef DEBUG
    Serial.println("Lock: KeyValid");
#endif

    counter_rolled = false;
    start = millis();
    next = start + unlock_time;
    if(next < start)
        counter_rolled = true;
}
void LockHandler::KeyValidReset() {
    digitalWrite(this->unlock_pin, LOW);
#ifdef DEBUG
    Serial.println("Lock: KeyValidReset");
#endif

    start = next = 0;
    counter_rolled = false;
}

void LockHandler::Reset() {
    digitalWrite(this->device_pin, LOW);
    digitalWrite(this->unlock_pin, LOW);
#ifdef DEBUG
    Serial.println("Lock: Reset");
#endif

    start = next = 0;
    counter_rolled = false;
}


void LockHandler::Task() {
    if(start == 0 && next == 0)
        return;

    start = millis();

    if(!counter_rolled) {
        if(start > next) {
            //Reset();
            KeyValidReset();
        }
    }
    else {
        if(start <= next)
            counter_rolled = false;
        if(start == next)
            Reset();
    }
}


void LockHandler::CheckInput(const uint8_t input_len, const char *input,
    const uint8_t serial_len, const uint8_t *serial) {
#ifdef ENABLE_UI
    char buf[100];
#endif
    Key_t key;
    boolean found = false;
    uint8_t otp_len = 0;
    uint8_t key_count = ReadKeyCount();
    uint16_t addr = ReadKeyStart();
    addr += sizeof(uint8_t);

#ifdef ENABLE_UI
    sprintf(buf, "Input: len (%d), (%s)", input_len, input);
    Serial.println(buf);

    E_Notify(PSTR("Serial: len ("), 0x80);
    Serial.print(serial_len);
    E_Notify(PSTR(") / ("), 0x80);
    for(int i=0; i<serial_len; i++) {
        PrintHex<uint8_t> (serial[i], 0x80);
        E_Notify(PSTR(" "), 0x80);
    }
    E_Notify(PSTR(")\r\n"), 0x80);
#endif

    for(int k=0; k<key_count && !found; k++) {
        otp_len = 0;

#ifdef ENABLE_UI
        sprintf(buf, "Checking key %d ... ", k+1);
        Serial.print(buf);
        buf[0] = 0;
#endif
        uint16_t keyLen = ReadKey(addr, &key);

        // Check if key is usable (enabled).
        if((key.state & KF_KEY_STATE) == KFS_DISABLED) {
#ifdef ENABLE_UI
            sprintf(buf + strlen(buf), "disabled");
#endif
            goto NEXT_KEY;
        }

        // Check if key has a serial number check constraint.
        if(key.serial_len > 0) {
            if(key.serial_len != serial_len) {
#ifdef ENABLE_UI
                sprintf(buf + strlen(buf), "serial length mismatch");
#endif
                goto NEXT_KEY;
            }
            if(memcmp(key.serial_bytes, serial, serial_len) != 0) {
#ifdef ENABLE_UI
                sprintf(buf + strlen(buf), "serial mismatch");
#endif
                goto NEXT_KEY;
            }
            else {
                // If serial number only key
                if((key.state & KF_KEY_TYPE) == KFT_SERIAL_NUMBER) {
                    found = true;
                    goto NEXT_KEY;
                }
            }
        }

        // Check type of key.
        switch(key.state & KF_KEY_TYPE) {


            // Check static keys
            case KFT_STATIC:
#ifdef ENABLE_UI
                sprintf(buf, "Static len %d", key.key_len);
#endif
                if(key.key_len != input_len) {
#ifdef ENABLE_UI
                    sprintf(buf + strlen(buf), " invalid mismatch input len %d",
                        input_len);
#endif
                    goto NEXT_KEY;
                }

                found = CheckStatic(&key, input);
#ifdef ENABLE_UI
                if(found)
                    sprintf(buf + strlen(buf), " OK");
#endif
                break;


            // Check HOTP keys
            case KFT_HMAC_OTP_LEN_6:
                otp_len = 6;
            case KFT_HMAC_OTP_LEN_8: {
                    otp_len = (otp_len == 0 ? 8 : otp_len);
#ifdef ENABLE_UI
                    sprintf(buf, "OTP len %d", otp_len);
#endif
                    // This should not happen but check regardless.
                    if(key.key_len != HMAC_KEY_LEN) {
#ifdef ENABLE_UI
                        sprintf(buf + strlen(buf), " invalid HMAC key len");
#endif
                        goto NEXT_KEY;
                    }

                    char *ptr;
                    unsigned long otp = strtoul(input, &ptr, 10);
                    ULL counter;
                    found = CheckOTP(addr, &key, otp_len, otp, &counter);
#ifdef ENABLE_UI
                    if(found)
                        sprintf(buf + strlen(buf), ", counter %lu OK",
                            counter);
#endif
                }
                break;

            default:
                break;
        }

NEXT_KEY:
#ifdef ENABLE_UI
        if(!found)
            sprintf(buf + strlen(buf), " FAILED");
        Serial.println(buf);
#endif

        InitKey(&key);
        addr += keyLen;
    }

    if(found) {
#ifdef ENABLE_UI
        Serial.println(F("*** KEY FOUND ***\r\n"));
#endif
        // signal valid key
        // red LED off, green led on for 3 sec
        RedLed.Off();
        GreenLed.Blink(UNLOCK_INDICATOR_TIME, 1);
        KeyValid();
    }
    else {
#ifdef ENABLE_UI
        Serial.println(F("*** KEY NOT FOUND ***\r\n"));
#endif
        // If only a check for Serial Number Only Key then do
        // not indicate Key Not Found in case no serial number matches
        // as a second run (when user presses token key for static
        // or HOTP keys) might find a correct key
        if(input_len > 0) {
            // signal invalid key, blink red LED for 10 sec, green LED off
            RedLed.Blink(ERROR_INDICATOR_TIME, ERROR_INDICATOR_REPEAT);
            GreenLed.Off();
        }
        else {
            // No Serial Only Key found - indicate waiting for other
            // keys (static, HOTP) by 4 green flashes
            RedLed.Off();
            GreenLed.Blink(250, 4);
        }
    }
}

#endif
