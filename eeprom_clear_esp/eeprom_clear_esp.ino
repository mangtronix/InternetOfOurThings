/*
 * EEPROM Clear for ESP8266 / ESP32
 *
 * Sets all of the bytes of the EEPROM to 0.
 * Please see eeprom_iteration for a more in depth
 * look at how to traverse the EEPROM.
 *
 * This example code is in the public domain.
 */

#include <EEPROM.h>

// Max size for ESP8266 https://arduino-esp8266.readthedocs.io/en/latest/libraries.html
#define EEPROM_SIZE 4096

void setup() {
  // initialize the LED pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  Serial.println();
  EEPROM.begin(EEPROM_SIZE);
  
  /***
    Iterate through each byte of the EEPROM storage.

    Larger AVR processors have larger EEPROM sizes, E.g:
    - Arduno Duemilanove: 512b EEPROM storage.
    - Arduino Uno:        1kb EEPROM storage.
    - Arduino Mega:       4kb EEPROM storage.

    Rather than hard-coding the length, you should use the pre-provided length function.
    This will make your code portable to all AVR processors.
  ***/

  for (int i = 0 ; i < EEPROM_SIZE ; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();

  // turn the LED on when we're done
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println("Cleared EEPROM");
}

void loop() {
  /** Empty loop. **/
}
