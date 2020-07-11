#ifndef _FLASHER_H
#define _FLASHER_H

#include <Arduino.h>

class Flasher
{
private:
  int _pin;
  int _onTime;
  int _offTime;

  uint8_t _isSingle;

  int _state;
  unsigned long _previousMillis;

public:
  Flasher(int pin, int on, int off);
  ~Flasher();

  void setTimes(int on, int off);

  void singleFlash();
  void reset();

  void update();

  void clear();
};

#endif // define _FLASHER_H