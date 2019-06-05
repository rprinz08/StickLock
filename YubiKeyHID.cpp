#include <limits.h>
#include <usbhid.h>
#include "global.h"
#include "led.h"
#include "eeprom.h"
#include "YubiKeyHID.h"

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
uint8_t YubiKeyHID::Init(uint8_t parent, uint8_t port, bool lowspeed) {
#ifdef ENABLE_UI
    Serial.println(F("\r\nUSB device connected ..."));
#endif

    uint8_t rcode = HIDUniversal::Init(parent, port, lowspeed);
    if(rcode != 0)
        return rcode;


    // vendor and product id
#ifdef ENABLE_UI
    E_Notify(PSTR("VID: 0x"), 0x80);
    PrintHex<uint16_t > (VID, 0x80);
    E_Notify(PSTR("\r\nPID: 0x"), 0x80);
    PrintHex<uint16_t > (PID, 0x80);
    E_Notify(PSTR("\r\n"), 0x80);
#endif
    DeviceVID = VID;
    DevicePID = PID;


    // Get device serial number.
    const uint8_t constBufSize = sizeof (USB_DEVICE_DESCRIPTOR);
    uint8_t buf[constBufSize];
    USB_DEVICE_DESCRIPTOR * udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR*>(buf);

    // Free prev serial buffer.
    if(DeviceSerial != NULL) {
        free(DeviceSerial);
        DeviceSerial = NULL;
    }

    // Get device descriptor.
    rcode = pUsb->getDevDescr(bAddress, 0, sizeof(USB_DEVICE_DESCRIPTOR), (uint8_t*)buf);
    if(rcode)
        goto NO_SERIAL_NUMBER;

    serialNumberIndex = udd->iSerialNumber;
    if(!serialNumberIndex)
        goto NO_SERIAL_NUMBER;

    uint8_t bufx[255];
    rcode = pUsb->getStrDescr(bAddress, 0, sizeof(bufx), serialNumberIndex, 0, bufx);
    if(rcode)
        goto NO_SERIAL_NUMBER;

    // Copy serial num to buffer.
    DeviceSerialLength = bufx[0] - 2;
    DeviceSerial = (uint8_t *)malloc(DeviceSerialLength);
    memcpy(DeviceSerial, &bufx[2], DeviceSerialLength);

#ifdef ENABLE_UI
    E_Notify(PSTR("SER: "), 0x80);
    Serial.print(DeviceSerialLength);
    E_Notify(PSTR(" / "), 0x80);
    for(int i=0; i<DeviceSerialLength; i++) {
        PrintHex<uint8_t> (DeviceSerial[i], 0x80);
        E_Notify(PSTR(" "), 0x80);
    }
    E_Notify(PSTR("\r\n"), 0x80);
#endif

    goto CHECK_DEVICE;


NO_SERIAL_NUMBER:

    serialNumberIndex = 0;
    DeviceSerialLength = 0;

    // Free previous serial number buffer.
    if(DeviceSerial != NULL) {
        free(DeviceSerial);
        DeviceSerial = NULL;
    }
#ifdef ENABLE_UI
    E_Notify(PSTR("SER: no serial number\r\n"), 0x80);
#endif


CHECK_DEVICE:
#ifndef DISABLE_SUPPORTED_DEVICE_CHECKS
    // Check for supported devices.
    DeviceSupported = false;
    Device_t device;
    uint8_t deviceCount = ReadDeviceCount();
    uint16_t addr = ReadDeviceStart();
    addr += sizeof(uint8_t);
    for(int d=0; d<deviceCount; d++) {
        uint16_t deviceLen = ReadDevice(addr, &device);
        if(device.pid == DevicePID && device.vid == DeviceVID) {
            DeviceSupported = true;
            break;
        }
        InitDevice(&device);
        addr += deviceLen;
    }

    if(DeviceSupported) {
#ifdef ENABLE_UI
        Serial.print(F("Supported device: "));
        Serial.println(device.name);
#endif
        InitDevice(&device);
        // green LED 4 times
        GreenLed.Blink(250, 4);
        Lock.DeviceSupported();
        PowerOff.Blink(1000, 60000, 1, LOW);
    }
    else {
#ifdef ENABLE_UI
        Serial.println(F("Unsupported device"));
#endif
        // red LED on continuous
        RedLed.On();
    }
#else
#ifdef ENABLE_UI
    Serial.print(F("Supported device: any device accepted"));
#endif
    // green LED 4 times
    GreenLed.Blink(250, 4);
    Lock.DeviceSupported();
    PowerOff.Blink(1000, 60000, 1, LOW);
#endif
#ifdef ENABLE_UI
    Serial.println();
#endif

    return 0;
}
#endif
