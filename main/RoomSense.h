#ifndef _ROOMSENSE_H
#define _ROOMSENSE_H

#include <Arduino.h>

class RoomSense
{
private:
  uint8_t _pinActivityLed;

public:
  RoomSense();
  ~RoomSense();

  /*!
    @brief  Set up a GPIO as a activity-indicator LED
    @param led The pin to use for the LED */
  void setActivityLed(uint8_t pin) { _pinActivityLed = pin; }

  void pulseActivityLed(int times);
};


#endif // define _ROOMSENSE_H