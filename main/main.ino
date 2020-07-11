#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include <OneBitDisplay.h>

#include "Flasher.h"
#include "RoomSense.h"

RoomSense room;
Flasher flash(2, 300, 170);

Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();

# define bme_addr 0x76

#define USE_BACKBUFFER

#ifdef USE_BACKBUFFER
static uint8_t ucBackBuffer[1024];
#else
static uint8_t *ucBackBuffer = NULL;
#endif

#define SDA_PIN 21
#define SCL_PIN 22
// Set this to -1 to disable or the GPIO pin number connected to the reset
// line of your display if it requires an external reset
#define RESET_PIN -1
// let OneBitDisplay figure out the display address
#define OLED_ADDR -1
// don't rotate the display
#define FLIP180 0
// don't invert the display
#define INVERT 0
// Bit-Bang the I2C bus
#define USE_HW_I2C 0x3C

// Change these if you're using a different OLED display
#define MY_OLED OLED_128x64
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

#define uS_TO_S_FACTOR  1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP   120       /* Time ESP32 will go to sleep (in seconds) */

#define TIME_TO_DISPLAY 10

#define Threshold 40

RTC_DATA_ATTR int bootCount = 0;
touch_pad_t touchPin;
OBDISP obd;
char data[128];
sensors_event_t temp_event, pressure_event, humidity_event;
int loopCount = 0;

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void print_wakeup_touchpad(){
  touch_pad_t pin;

  touchPin = esp_sleep_get_touchpad_wakeup_status();

  switch(touchPin)
  {
    case 0  : Serial.println("Touch detected on GPIO 4"); break;
    case 1  : Serial.println("Touch detected on GPIO 0"); break;
    case 2  : Serial.println("Touch detected on GPIO 2"); break;
    case 3  : Serial.println("Touch detected on GPIO 15"); break;
    case 4  : Serial.println("Touch detected on GPIO 13"); break;
    case 5  : Serial.println("Touch detected on GPIO 12"); break;
    case 6  : Serial.println("Touch detected on GPIO 14"); break;
    case 7  : Serial.println("Touch detected on GPIO 27"); break;
    case 8  : Serial.println("Touch detected on GPIO 33"); break;
    case 9  : Serial.println("Touch detected on GPIO 32"); break;
    default : Serial.println("Wakeup not by touchpad"); break;
  }
}

void callback(){
  flash.singleFlash();
  loopCount = 0;
}

void setup() {
  Serial.begin(115200);

  ++bootCount;
  print_wakeup_reason();
  print_wakeup_touchpad();

  //Setup interrupt on Touch Pad 3 (GPIO15)
  touchAttachInterrupt(T0, callback, Threshold);

  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();

  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  int rc;
  rc = obdI2CInit(&obd, MY_OLED, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, SDA_PIN, SCL_PIN, RESET_PIN, 800000L); // use standard I2C bus at 400Khz
  if (rc != OLED_NOT_FOUND)
  {
    char *msgs[] = {(char *)"SSD1306 @ 0x3C", (char *)"SSD1306 @ 0x3D",(char *)"SH1106 @ 0x3C",(char *)"SH1106 @ 0x3D"};
    obdFill(&obd, 0, 1);
    obdWriteString(&obd, 0,0,0,msgs[rc], FONT_NORMAL, 0, 1);
    obdSetBackBuffer(&obd, ucBackBuffer);
    delay(2000);
  }

  if (!bme.begin(bme_addr)) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1) delay(10);
  }
}

void loop() {
  obdPower(&obd, 1);

  obdFill(&obd, 0, 1);
  for (loopCount = 0; loopCount < TIME_TO_DISPLAY; loopCount++)
  {
    senseData();
    displayData();
    delay(1000);
  }

  obdPower(&obd, 0);
  flash.clear();
  esp_deep_sleep_start();
}

void senseData() {
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);
}

void displayData() {
  sprintf(data, "Temp.   : %.1f C", temp_event.temperature);
  obdWriteString(&obd, 0,0,0, data, FONT_SMALL, 0, 1);

  sprintf(data, "Humidity: %.1f P", humidity_event.relative_humidity);
  obdWriteString(&obd, 0,0,1, data, FONT_SMALL, 0, 1);

  sprintf(data, "Preasure: %.1f hPa", pressure_event.pressure);
  obdWriteString(&obd, 0,0,2, data, FONT_SMALL, 0, 1);

  float ll = 0;
  float lm = 127;
  int lf = floor(127 / TIME_TO_DISPLAY);
  int p = lf * loopCount;

  obdDrawLine(&obd, lm,62, p,62, 1,1);

}