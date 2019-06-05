#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <Arduino.h>
#include <EEPROM.h>
#include "sha1.h"
#include "eeprom.h"
#include "global.h"
#include "led.h"
#include "hmac.h"

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

/*
 * see https://tools.ietf.org/html/rfc4226#section-5.4
 * for more details
 */

#ifndef CONFIG
// ipow returns based raised to the power of exp.
ULL ipow(ULL base, int exp) {
    unsigned long long int result = 1ULL;
    while(exp) {
        if(exp & 1)
            result *= (ULL)base;
        exp >>= 1;
        base *= base;
    }
    return result;
}


// CalcOTP calculates 6 or 8 byte (specified by otpLen) HOTP's from
// provided key and counter values.
unsigned long CalcOTP(uint8_t keyLen, uint8_t *key, ULL counter, uint8_t otpLen) {
    union {
        ULL cnt;
        uint8_t bytes[sizeof(ULL)];
    } cnt_bytes;
    cnt_bytes.cnt = counter;

    Sha1.initHmac(key, keyLen);
    for (int i=7; i>=0; i--)
        Sha1.write(cnt_bytes.bytes[i]);
    uint8_t *hash = Sha1.resultHmac();

    uint8_t offset = (hash[19] & 0x0f);
    unsigned long bin_code =
        ((long)hash[offset] & 0x7f) << 24 |
        ((long)hash[offset+1] & 0xff) << 16 |
        ((long)hash[offset+2] & 0xff) << 8 |
        (long)hash[offset+3] & 0xff;
    // XXX: dont use this - can give wrong resilts !
    //unsigned long p = (unsigned long)(pow(10, otpLen));
    ULL p = ipow(10, otpLen);
    unsigned long otp = bin_code % p;
    return otp;
}


boolean CheckStatic(Key_t *key, const char *phrase) {
    for(int i=0; i<key->key_len; i++)
        if(key->key_bytes[i] != phrase[i])
            return false;

    return true;
}


// CheckOTP checks provided OTP against all enabled HOTP
// keys configured in EEPROM.
boolean CheckOTP(uint16_t addr, Key_t *key, uint8_t otpLen,
        unsigned long otp, ULL *counter) {
    boolean found = false;
    ULL maxCounter = key->counter + key->counter_tolerance;

    *counter = 0;
    for(int i=key->counter; i<maxCounter; i++) {
        unsigned long keyOTP = CalcOTP(key->key_len, key->key_bytes, i, otpLen);
        if(keyOTP == otp) {
            *counter = i;
            key->counter = ++i;
            WriteKey(addr, key, false);
            found = true;
            break;
        }
    }

    return found;
}

#endif
