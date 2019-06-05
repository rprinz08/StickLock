#ifndef _LED_H_
#define _LED_H_


class LedHandler {
private:
    unsigned long start;
    unsigned long next;
    boolean counter_rolled;
    boolean initial_led_state;

    void toggle();

protected:
    uint8_t pin;
    unsigned long led_on_duration;
    unsigned long led_off_duration;
    unsigned long led_repeat;
    boolean led_state;

public:
    LedHandler(uint8_t ledPin);
    void Init();
    void Init(uint8_t ledPin);
    void On();
    void Off();
    void Blink(unsigned long duration, int repeat);
    void Blink(unsigned long on_duration, unsigned long off_duration,
        int repeat, boolean initial_led_state);

    void Task();
};


#endif
