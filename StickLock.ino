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

// ============================================================================
#ifdef CONFIG
// Run in config mode to flash key information to EEPROM
// See also comments in "config.h" for more information.

#include <EEPROM.h>
#include "eeprom.h"
#include "config.h"

#define FLAG_SHOW_PROMPT    1
#define SHOW_PROMPT         ((prgFlags & FLAG_SHOW_PROMPT) == FLAG_SHOW_PROMPT)

byte prgFlags = FLAG_SHOW_PROMPT;
boolean yes = false;


void usage(const char c) {
    Serial.println();
    if(c) {
        char buf[50];
        snprintf(buf, 32, "*** Unknown command (%c) ***\r\n", c);
        Serial.println(buf);
    }
    Serial.println(F(
        "Usage: \r\n"
        "h : shows this usage information\r\n"
        "D : print configured devices\r\n"
        "d : print EEPROM devices\r\n"
        "K : print configured keys\r\n"
        "k : print EEPROM keys\r\n"
        "c : export devices & keys in EEPROM as C code\r\n"
        "w : write configured devices and keys to EEPROM\r\n"
        "X : erase EEPROM!!\r\n"
        "E : show EEPROM hex dump"));
}


// askYesNo shows prompt and waits until user enters yes or no.
// Yes could be (y,Y,j,J) and No (n,N)
// Return true for YES and false for NO
boolean askYesNo(const char *prompt) {
    uint8_t inByte;
    Serial.print(prompt);
    while(true) {
        if(Serial.available() > 0) {
            inByte = Serial.read();
            if(inByte == 'Y' || inByte == 'J' || inByte == 'y' || inByte == 'j') {
                Serial.println("Y");
                return true;
            }
            if(inByte == 'N' || inByte == 'n') {
                Serial.println("N");
                return false;
            }
        }
        delay(200);
    }
}


void setup() {
    Serial.begin(115200);
    Serial.println(F("StickLock " VERSION " configuration mode ...\r\n"));

    pinMode(RED_LED_PIN, OUTPUT);
    digitalWrite(RED_LED_PIN, HIGH);
    pinMode(GREEN_LED_PIN, OUTPUT);
    digitalWrite(GREEN_LED_PIN, HIGH);

    usage(0);
}


void loop() {
    uint8_t inByte;

    if(SHOW_PROMPT)
        Serial.print(">");

    if(Serial.available() > 0) {
        inByte = Serial.read();
        Serial.write(inByte);
        Serial.println();

        switch(inByte) {
            case '?':
            case 'h':
                usage(0);
                break;

            case 'w':
                yes = askYesNo("Write configured devices and keys to "
                    "EEPROM - are you sure (Y/N): ");
                if(yes) {
                    Serial.println();
                    InitEEPROM();
                    Serial.println(F("Now you can disable config mode and flash "
                        "again for normal operation ..."));
                }
                break;

            case 'd':
                Serial.println(F("EEPROM devices ..."));
                PrintDevices();
                break;

            case 'D':
                Serial.println(F("Configured devices ..."));
                PrintWriteConfigDevices(false, false, 0, NULL);
                break;

            case 'k':
                Serial.println(F("EEPROM keys ..."));
                PrintKeys();
                break;

            case 'K':
                Serial.println(F("Configured keys ..."));
                PrintWriteConfigKeys(false, false, 0, NULL);
                break;

            case 'X':
                yes = askYesNo("Erase EEPROM - are you sure (Y/N): ");
                if(yes) {
                    Serial.println();
                    ClearEEPROM();
                    Serial.println(F("EEPROM erased!"));
                }
                break;

            case 'E':
                DumpEEPROM(0, EEPROM.length());
                break;

            case 'c':
                Serial.println(F("\r\nThe output below can be copy/pasted into config.h\r\n"));
                ExportDevicesAndKeys();
                break;

            default:
                usage(inByte);
                break;
        }
        Serial.println();
        prgFlags |= FLAG_SHOW_PROMPT;
    }
    else {
        prgFlags &= ~(FLAG_SHOW_PROMPT);
        delay(200);
    }
}


// ============================================================================
#else
// Run in normal (LOCK) mode.

#include <limits.h>
#include "eeprom.h"
#include "lock.h"
#include "led.h"
#include "YubiKeyHID.h"
#include "YubiKeyReportParser.h"
#include "YubiKeyReportDescParser.h"


char inputBuffer[MAX_INPUT_LEN];
uint8_t inputPtr;
uint8_t inputLen;
LedHandler RedLed(RED_LED_PIN);
LedHandler GreenLed(GREEN_LED_PIN);
LedHandler PowerOff(POWER_OFF_PIN);
uint8_t lastUsbState = 0;
USB Usb;
YubiKeyHID Hid(&Usb);
YubiKeyReportParser Uni;
LockHandler Lock(DEVICE_PIN, UNLOCK_PIN, UNLOCK_TIME);

#ifndef DISABLE_CLEAR_SWITCH
boolean in_clear;
unsigned long clear_last;
unsigned long clear_acc;
#endif

void setup() {
#ifdef ENABLE_UI
    Serial.begin(115200);
#endif

#ifndef DISABLE_CLEAR_SWITCH
    pinMode(CLEAR_SWITCH_PIN, INPUT);
    in_clear = false;
#endif    

    // Set default power off to 1 minute.
    PowerOff.Blink(1000, 60000, 1, LOW);

#ifdef ENABLE_UI
    Serial.println(F("StickLock " VERSION ""));
#endif

    // Check if SL is configured correctly.
    uint16_t addr = ReadDeviceStart();
    if(addr != (EEPROM_KEY_START + sizeof(uint16_t))) {
#ifdef ENABLE_UI
        Serial.println(F("*** NOT CONFIGURED ***"));
#endif
        digitalWrite(RED_LED_PIN, HIGH);
        while(true) {}
    }

    if(Usb.Init() == -1) {
#ifdef ENABLE_UI
        Serial.println(F("OSC did not start"));
#else
        digitalWrite(RED_LED_PIN, HIGH);
        while(true) {}
#endif
    }

    delay(200);

    if(!Hid.SetReportParser(0, &Uni)) {
#ifdef ENABLE_UI
        ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1);
#else
        digitalWrite(RED_LED_PIN, HIGH);
        while(true) {}
#endif
    }
}


void loop() {
    // Handle USB.
    Usb.Task();

    // Detect device disconnect.
    uint8_t usbState = Usb.getUsbTaskState();
    if(usbState != lastUsbState) {
        lastUsbState = usbState;
        if(usbState == USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE) {
            RedLed.Off();
            GreenLed.Off();
            Lock.Reset();
            // Power off 10 seconds after device is disconnected.
            PowerOff.Blink(1000, 10000, 1, LOW);
        }
    }

    // Handle LED indicators.
    RedLed.Task();
    GreenLed.Task();

    // Handle Lock/Unlock.
    Lock.Task();

    // Handle Poweroff.
    PowerOff.Task();

#ifndef DISABLE_CLEAR_SWITCH
    // Handle clear button.
    if(digitalRead(CLEAR_SWITCH_PIN) == LOW) {
        in_clear = false;
        clear_acc = 0;
    }    
    else {
        if(in_clear) {
            unsigned long clear_now = millis();
            if(clear_now < clear_last)
                clear_acc += (ULONG_MAX - clear_last) + clear_now;
            else
                clear_acc += clear_now - clear_last;
            if(clear_acc >= CLEAR_SWITCH_ACCEPT) {
#ifdef ENABLE_UI                
                Serial.println(F("*** CLEAR ***"));
#endif                
                digitalWrite(RED_LED_PIN, HIGH);
                ClearEEPROM();
                while(true) {}                
                //in_clear = false;
                //clear_acc = 0;
            }
            else
                clear_last = clear_now;
        }
        else {
            in_clear = true;
            clear_last = millis();
        }
    }
#endif
}
#endif
