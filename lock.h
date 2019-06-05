#ifndef _LOCK_H_
#define _LOCK_H_

#ifndef CONFIG
class LockHandler {
private:
    uint8_t device_pin;
    uint8_t unlock_pin;
    unsigned long unlock_time;
    unsigned long start;
    unsigned long next;
    boolean counter_rolled;

protected:

public:
    LockHandler(uint8_t device_pin, uint8_t unlock_pin,
        unsigned long unlock_time);
    void DeviceSupported();
    void DeviceSupportedReset();
    void KeyValid();
    void KeyValidReset();
    void Reset();
    void CheckInput(const uint8_t input_len, const char *input,
        const uint8_t serial_len, const uint8_t *serial);
    void Task();
};
#endif

#endif
