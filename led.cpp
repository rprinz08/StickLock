#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <Arduino.h>
#include "global.h"
#include "led.h"

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

LedHandler::LedHandler(uint8_t ledPin) {
    pin = ledPin;
    pinMode(pin, OUTPUT);
    Off();
}


void LedHandler::On() {
    led_on_duration = 0;
    led_off_duration = 0;
    led_repeat = 0;
    led_state = HIGH;
    digitalWrite(pin, led_state);
}


void LedHandler::Off() {
    led_on_duration = 0;
    led_off_duration = 0;
    led_repeat = 0;
    led_state = LOW;
    digitalWrite(pin, led_state);
}


void LedHandler::Blink(unsigned long duration, int repeat) {
    Blink(duration, duration, repeat, HIGH);
}
void LedHandler::Blink(unsigned long on_duration, unsigned long off_duration,
        int repeat, boolean initial_led_state) {
    led_on_duration = on_duration;
    led_off_duration = off_duration;
    led_repeat = repeat;
    led_state = initial_led_state;
    this->initial_led_state = initial_led_state;
    digitalWrite(pin, led_state);

    counter_rolled = false;
    start = millis();
    next = start + (led_state == HIGH ? led_on_duration : led_off_duration);
    if(next < start)
        counter_rolled = true;
}


void LedHandler::toggle() {
    led_state = !led_state;
    digitalWrite(pin, led_state);

    if(led_state == initial_led_state)
        led_repeat--;
    if(led_repeat < 1) {
        led_state = LOW;
        digitalWrite(pin, led_state);
        return;
    }

    start = millis();
    next = start + (led_state == HIGH ? led_on_duration : led_off_duration);
    if(next < start)
        counter_rolled = true;
}


void LedHandler::Task() {
    if(led_repeat < 1)
        return;

    start = millis();

    if(!counter_rolled) {
        if(start > next)
            toggle();
    }
    else {
        if(start <= next)
            counter_rolled = false;
        if(start == next)
            toggle();
    }
}
