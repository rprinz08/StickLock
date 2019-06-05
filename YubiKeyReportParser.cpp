#include <limits.h>
#include <usbhid.h>
#include "global.h"
#include "led.h"
#include "eeprom.h"
#include "YubiKeyHID.h"
#include "YubiKeyReportParser.h"
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
void YubiKeyReportParser::Parse(USBHID *hid, bool is_rpt_id __attribute__((unused)),
        uint8_t len, uint8_t *buf) {

    YubiKeyHID *yh = reinterpret_cast<YubiKeyHID*>(hid);
#ifndef DISABLE_SUPPORTED_DEVICE_CHECKS
    if(!yh->DeviceSupported)
       return;
#endif

    YubiKeyReportDescParser prs(yh, len, buf);
    uint8_t ret = hid->GetReportDescr(0, &prs);

#ifdef DEBUG
    E_Notify(PSTR("\r\n"), 0x80);
#endif

    if(ret)
        ErrorMessage<uint8_t> (PSTR("YubiKeyReportParser"), ret);
}
#endif
