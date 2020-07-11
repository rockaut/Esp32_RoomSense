#include "Flasher.h"

#include <Arduino.h>

Flasher::Flasher(int pin, int on, int off)
{
  _isSingle = 0;

  _pin = pin;
  pinMode(_pin, OUTPUT);

  _onTime = on;
  _offTime = off;

  _state = LOW;
  _previousMillis = 0;
}

Flasher::~Flasher()
{
}

void Flasher::setTimes(int on, int off) {
  _onTime = on;
  _offTime = off;
}

void Flasher::singleFlash() {
  if(_isSingle <= 0) { _isSingle = 1; }
  update();
}

void Flasher::update() {
  unsigned long currentMillis = millis();

  if(_isSingle > 1) {
    _previousMillis = currentMillis;
    return;
  }

  if((_state == HIGH) && (currentMillis - _previousMillis >= _onTime)) {
    _state = LOW;
    _previousMillis = currentMillis;
    digitalWrite(_pin, _state);
    if(_isSingle > 0) {
      _isSingle = 2;
    }
  }
  else if ((_state == LOW) && (currentMillis - _previousMillis >= _offTime)) {
    _state = HIGH;
    _previousMillis = currentMillis;
    digitalWrite(_pin, _state);
  }
}

void Flasher::reset() {
  _isSingle = 0;
}

void Flasher::clear() {
  reset();

  _state = LOW;
  _previousMillis = millis();
  digitalWrite(_pin, _state);
}
