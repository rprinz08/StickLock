#ifndef _YubiKeyReportParser_h_
#define _YubiKeyReportParser_h_

#ifndef CONFIG
#include <usbhid.h>
#include <hiduniversal.h>

class YubiKeyReportParser : public HIDReportParser {
public:
    virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};
#endif

#endif
