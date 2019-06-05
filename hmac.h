#ifndef _HMAC_H_
#define _HMAC_H_

#include "global.h"

#ifndef CONFIG
extern unsigned long CalcOTP(uint8_t keyLen, uint8_t *key, ULL counter, uint8_t otpLen);
//extern void CheckInput(const uint8_t input_len, const char *input,
//    const uint8_t serialLen, const uint8_t *serial);
extern boolean CheckOTP(uint16_t addr, Key_t *key, uint8_t otpLen,
    unsigned long otp, ULL *counter);
extern boolean CheckStatic(Key_t *key, const char *phrase);
extern LedHandler RedLed;
extern LedHandler GreenLed;
#endif

#endif
