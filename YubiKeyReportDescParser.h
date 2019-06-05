#ifndef _YubiKeyReportDescParser_h_
#define _YubiKeyReportDescParser_h_

#ifndef CONFIG
#include "lock.h"
#include "YubiKeyHID.h"

#define UHS_HID_BOOT_KEY_ZERO           0x27
#define UHS_HID_BOOT_KEY_ENTER          0x28
#define UHS_HID_BOOT_KEY_SPACE          0x2c
#define UHS_HID_BOOT_KEY_CAPS_LOCK      0x39
#define UHS_HID_BOOT_KEY_SCROLL_LOCK    0x47
#define UHS_HID_BOOT_KEY_NUM_LOCK       0x53
#define UHS_HID_BOOT_KEY_ZERO2          0x62
#define UHS_HID_BOOT_KEY_PERIOD         0x63

struct KBDLEDS {
    uint8_t bmNumLock : 1;
    uint8_t bmCapsLock : 1;
    uint8_t bmScrollLock : 1;
    uint8_t bmCompose : 1;
    uint8_t bmKana : 1;
    uint8_t bmReserved : 3;
};

extern char inputBuffer[MAX_INPUT_LEN];
extern uint8_t inputPtr;
extern uint8_t inputLen;
extern LedHandler RedLed;
extern LedHandler GreenLed;
extern LedHandler PowerOff;
extern LockHandler Lock;


class YubiKeyReportDescParser : public USBReadParser {
    // Report ID
    uint8_t rptId;

    // Usage Minimum
    uint8_t useMin;

    // Usage Maximum
    uint8_t useMax;

    // Number of field being currently processed
    uint8_t fieldCount;

    // Method which is called every time Input item is found
    void OnInputItem(uint8_t itm);

    // Report buffer pointer
    uint8_t *pBuf;

    // Report length
    uint8_t bLen;

    uint8_t y_mod;
    uint8_t y_scan;

    static const uint8_t numKeys[10];
    static const uint8_t symKeysUp[12];
    static const uint8_t symKeysLo[12];
    static const uint8_t padKeys[5];

public:
    typedef void (*UsagePageFunc)(uint16_t usage);
    YubiKeyHID *hid;

protected:
    MultiValueBuffer theBuffer;
    MultiByteValueParser valParser;
    ByteSkipper theSkipper;
    uint8_t varBuffer[sizeof (USB_CONFIGURATION_DESCRIPTOR)];
    uint8_t itemParseState;
    uint8_t itemSize;
    uint8_t itemPrefix;
    uint8_t rptSize; // Report Size
    uint8_t rptCount; // Report Count
    uint16_t totalSize; // Report size in bits
    //UsagePageFunc pfUsage;

    //void SetUsagePage(uint16_t page);

    // Method should be defined here if virtual.
    virtual uint8_t ParseItem(uint8_t **pp, uint16_t *pcntdn);
    uint8_t OemToAscii(uint8_t mod, uint8_t key);

    union {
        KBDLEDS kbdLeds;
        uint8_t bLeds;
    } kbdLockingKeys;

    virtual const uint8_t *getNumKeys() {
        return numKeys;
    };

    virtual const uint8_t *getSymKeysUp() {
        return symKeysUp;
    };

    virtual const uint8_t *getSymKeysLo() {
        return symKeysLo;
    };

    virtual const uint8_t *getPadKeys() {
        return padKeys;
    };

public:
    enum {
        enErrorSuccess = 0,
        // value or record is partialy read in buffer
        enErrorIncomplete,
        enErrorBufferTooSmall
    };

    YubiKeyReportDescParser(YubiKeyHID *yhid, uint16_t len, uint8_t *pbuf) :
        USBReadParser(),
        itemParseState(0), itemSize(0), itemPrefix(0),
        rptSize(0), rptCount(0), rptId(0),
        useMin(0), useMax(0),
        fieldCount(0), pBuf(pbuf), bLen(len) {
            theBuffer.pValue = varBuffer;
            valParser.Initialize(&theBuffer);
            theSkipper.Initialize(&theBuffer);
            hid = yhid;
    };

    void Parse(const uint16_t len, const uint8_t *pbuf, const uint16_t &offset);
};
#endif

#endif
