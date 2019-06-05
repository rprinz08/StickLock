#ifndef _EEPROM_H_
#define _EEPROM_H_

#include "global.h"

void ClearEEPROM();
#ifdef CONFIG
void DumpEEPROM(uint16_t addr, uint16_t size);
#endif

extern uint16_t Write16(uint16_t addr, uint16_t value);
extern uint16_t Read16(uint16_t addr, uint16_t *value);

extern void InitDevice(Device_t *device);
extern uint16_t WriteDeviceStart(uint16_t startAddr);
extern uint16_t ReadDeviceStart();
extern void WriteDeviceCount(uint8_t deviceCount);
extern uint8_t ReadDeviceCount();
extern uint16_t WriteDevice(uint16_t addr, Device_t *device, boolean calcOnly);
extern uint16_t ReadDevice(uint16_t addr, Device_t *device);

extern void InitKey(Key_t *key);
extern uint16_t WriteKeyStart(uint16_t startAddr);
extern uint16_t ReadKeyStart();
extern void WriteKeyCount(uint8_t keyCount);
extern uint8_t ReadKeyCount();
extern uint16_t WriteKey(uint16_t addr, const Key_t *key, boolean calcOnly);
extern uint16_t ReadKey(uint16_t addr, Key_t *key);

#endif
