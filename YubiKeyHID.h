#ifndef _YubiKeyHID_h_
#define _YubiKeyHID_h_

#include <usbhid.h>
#include <hiduniversal.h>
#include "lock.h"

#ifndef CONFIG
extern LedHandler RedLed;
extern LedHandler GreenLed;
extern LedHandler PowerOff;
extern LockHandler Lock;

class YubiKeyHID : public HIDUniversal {
public:
    YubiKeyHID(USB *usb): HIDUniversal(usb) {};
    uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed);

    // PID and VID of connected device
    uint16_t DevicePID, DeviceVID;

    // Device serial number (if any)
    uint8_t *DeviceSerial = NULL;
    uint8_t DeviceSerialLength = 0;

#ifndef DISABLE_SUPPORTED_DEVICE_CHECKS
    // true if device is supported
    boolean DeviceSupported = false;
#endif

protected:
    uint8_t serialNumberIndex = 0;
};
#endif

#endif
