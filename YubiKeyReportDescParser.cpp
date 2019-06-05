#include <stdlib.h>
#include <usbhid.h>
#include <hiduniversal.h>
#include <hidescriptorparser.h>
#include "global.h"
#include "led.h"
#include "YubiKeyReportDescParser.h"

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

const uint8_t YubiKeyReportDescParser::numKeys[10] PROGMEM = {
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')'};

const uint8_t YubiKeyReportDescParser::symKeysUp[12] PROGMEM = {
    '_', '+', '{', '}', '|', '~', ':', '"', '~', '<', '>', '?'};

const uint8_t YubiKeyReportDescParser::symKeysLo[12] PROGMEM = {
    '-', '=', '[', ']', '\\', ' ', ';', '\'', '`', ',', '.', '/'};

const uint8_t YubiKeyReportDescParser::padKeys[5] PROGMEM = {
    '/', '*', '-', '+', 0x13};


void YubiKeyReportDescParser::Parse(const uint16_t len, const uint8_t *pbuf,
        const uint16_t &offset __attribute__((unused))) {

    uint16_t cntdn = (uint16_t)len;
    uint8_t *p = (uint8_t*)pbuf;

    totalSize = 0;

    while(cntdn) {
        ParseItem(&p, &cntdn);
    }
}


uint8_t YubiKeyReportDescParser::ParseItem(uint8_t **pp, uint16_t *pcntdn) {
    switch (itemParseState) {
        case 0:
            if (**pp != HID_LONG_ITEM_PREFIX) {
                uint8_t size = ((**pp) & DATA_SIZE_MASK);
                itemPrefix = (**pp);
                itemSize = 1 + ((size == DATA_SIZE_4) ? 4 : size);
            }
            (*pp)++;
            (*pcntdn)--;
            itemSize--;
            itemParseState = 1;

            if (!itemSize)
                break;

            if (!pcntdn)
                return enErrorIncomplete;

        case 1:
            theBuffer.valueSize = itemSize;
            valParser.Initialize(&theBuffer);
            itemParseState = 2;

        case 2:
            if (!valParser.Parse(pp, pcntdn))
                return enErrorIncomplete;
            itemParseState = 3;

        case 3:
        {
            uint8_t data = *((uint8_t*)varBuffer);

            switch (itemPrefix & (TYPE_MASK | TAG_MASK)) {
                case (TYPE_LOCAL | TAG_LOCAL_USAGE):
                    if(data == 6) {
#ifdef DEBUG
                        // 0x06 = keypad
                        E_Notify(PSTR("{"), 0x80);
                        PrintHex<uint8_t > (data, 0x80);
                        E_Notify(PSTR("}   "), 0x80);
#endif
                        y_mod = 0;
                        y_scan = 0;
                    }
                    break;

                case (TYPE_GLOBAL | TAG_GLOBAL_REPORTSIZE):
                    rptSize = data;
                    break;

                case (TYPE_GLOBAL | TAG_GLOBAL_REPORTCOUNT):
                    rptCount = data;
                    break;

                case (TYPE_GLOBAL | TAG_GLOBAL_REPORTID):
                    rptId = data;
                    break;

                case (TYPE_LOCAL | TAG_LOCAL_USAGEMIN):
                    useMin = data;
                    break;

                case (TYPE_LOCAL | TAG_LOCAL_USAGEMAX):
                    useMax = data;
                    break;

                case (TYPE_MAIN | TAG_MAIN_OUTPUT):
                case (TYPE_MAIN | TAG_MAIN_FEATURE):
                    rptSize = 0;
                    rptCount = 0;
                    useMin = 0;
                    useMax = 0;
                    break;

                case (TYPE_MAIN | TAG_MAIN_INPUT):
                    OnInputItem(data);
                    totalSize += (uint16_t)rptSize * (uint16_t)rptCount;
                    rptSize = 0;
                    rptCount = 0;
                    useMin = 0;
                    useMax = 0;
                    break;
            }
        }
    }

    itemParseState = 0;
    return enErrorSuccess;
}


void YubiKeyReportDescParser::OnInputItem(uint8_t itm) {
    // calculate offset to the next unhandled byte i = (int)(totalCount / 8);
    uint8_t byte_offset = (totalSize >> 3);
    uint32_t tmp = (byte_offset << 3);
    // number of bits in the current byte already handled
    uint8_t bit_offset = totalSize - tmp;
    // current byte pointer
    uint8_t *p = pBuf + byte_offset;

    if (bit_offset)
        *p >>= bit_offset;

    uint8_t usage = useMin;

    uint8_t bits_of_byte = 8;

    switch(itm) {
        case 2:
            y_mod = *p;
            break;
        case 0:
            y_scan = *p;
            break;
    }

#ifdef DEBUG
    E_Notify(PSTR("["), 0x80);
    PrintHex<uint8_t > (itm, 0x80);
    E_Notify(PSTR(","), 0x80);
    PrintHex<uint8_t > (*p, 0x80);
    E_Notify(PSTR("] "), 0x80);
#endif

    for (uint8_t field = 0; field < rptCount; field++, usage++) {
        union {
            uint8_t bResult[4];
            uint16_t wResult[2];
            uint32_t dwResult;
        } result;

        result.dwResult = 0;
        uint8_t mask = 0;

        // bits_left    - number of bits in the field(array of fields, depending on Report Count) left to process
        // bits_of_byte - number of bits in current byte left to process
        // bits_to_copy - number of bits to copy to result buffer

        // for each bit in a field
        for (uint8_t bits_left = rptSize, bits_to_copy = 0; bits_left; bits_left -= bits_to_copy) {
            bits_to_copy = (bits_left > bits_of_byte) ? bits_of_byte : bits_left;

            // Result buffer is shifted by the number of bits to be copied in
            result.dwResult <<= bits_to_copy;

            uint8_t val = *p;

            // Shift by the number of bits already processed
            val >>= (8 - bits_of_byte);

            mask = 0;

            for (uint8_t j = bits_to_copy; j; j--) {
                mask <<= 1;
                mask |= 1;
            }

            result.bResult[0] = (result.bResult[0] | (val & mask));

            bits_of_byte -= bits_to_copy;

            if (bits_of_byte < 1) {
                bits_of_byte = 8;
                p++;
            }
        }
#ifdef DEBUG
        PrintHex<uint8_t > (result.dwResult, 0x80);
        E_Notify(PSTR(" "), 0x80);
#endif
    }

#ifdef DEBUG
    E_Notify(PSTR("   "), 0x80);
#endif

    if(itm == 0 && y_scan != 0) {
        // red LED on continous during USB activity
        RedLed.On();
        // green LED off
        GreenLed.Off();
        // As long as there is USB activity initialize auto power off to 1 minute
        PowerOff.Blink(1000, 60000, 1, LOW);

        uint8_t ch = OemToAscii(y_mod, y_scan);
#ifdef DEBUG
        char buf[16];
        sprintf(buf, "<%02x,%02x,%02x,%c>", y_mod, y_scan, ch, ch);
        Serial.print(buf);
#endif
        if(ch != END_OF_INPUT_CHAR) {
            inputBuffer[inputPtr++] = ch;
            inputLen++;
        }

        if(inputLen >= (MAX_INPUT_LEN - 1))
            ch = END_OF_INPUT_CHAR;

        if(ch == END_OF_INPUT_CHAR) {
            inputBuffer[inputPtr] = 0;
#ifdef DEBUG
            Serial.println();
            Serial.println();
            Serial.print("Entered key: ");
            Serial.println((char *)&inputBuffer);
#endif
            Lock.CheckInput(inputLen, inputBuffer,
                hid->DeviceSerialLength, hid->DeviceSerial);

            inputPtr = 0;
            inputLen = 0;
        }
    }
}


uint8_t YubiKeyReportDescParser::OemToAscii(uint8_t mod, uint8_t key) {
    uint8_t shift = (mod & 0x22);

    // [a-z]
    if (VALUE_WITHIN(key, 0x04, 0x1d)) {
        // Upper case letters
        if ((kbdLockingKeys.kbdLeds.bmCapsLock == 0 && shift) ||
                (kbdLockingKeys.kbdLeds.bmCapsLock == 1 && shift == 0))
            return (key - 4 + 'A');
        // Lower case letters
        else
            return (key - 4 + 'a');
    }
    // Numbers
    else if (VALUE_WITHIN(key, 0x1e, 0x27)) {
        if (shift)
            return ((uint8_t)pgm_read_byte(&getNumKeys()[key - 0x1e]));
        else
            return ((key == UHS_HID_BOOT_KEY_ZERO) ? '0' : key - 0x1e + '1');
    }
    // Keypad Numbers
    else if (VALUE_WITHIN(key, 0x59, 0x61)) {
        if (kbdLockingKeys.kbdLeds.bmNumLock == 1)
            return (key - 0x59 + '1');
    }
    else if (VALUE_WITHIN(key, 0x2d, 0x38))
        return ((shift) ?
            (uint8_t)pgm_read_byte(&getSymKeysUp()[key - 0x2d]) :
            (uint8_t)pgm_read_byte(&getSymKeysLo()[key - 0x2d]));
    else if(VALUE_WITHIN(key, 0x54, 0x58))
        return (uint8_t)pgm_read_byte(&getPadKeys()[key - 0x54]);
    else {
        switch(key) {
            case UHS_HID_BOOT_KEY_SPACE:
                return (0x20);

            case UHS_HID_BOOT_KEY_ENTER:
                return (0x13);

            case UHS_HID_BOOT_KEY_ZERO2:
                return ((kbdLockingKeys.kbdLeds.bmNumLock == 1) ? '0': 0);

            case UHS_HID_BOOT_KEY_PERIOD:
                return ((kbdLockingKeys.kbdLeds.bmNumLock == 1) ? '.': 0);
        }
    }
    return (0);
}

#endif
