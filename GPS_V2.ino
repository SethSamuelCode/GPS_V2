#include <Wire.h>                  //for screen
#include <Adafruit_GFX.h>          //for screen
#include "Adafruit_LEDBackpack.h"  //for screen
#include <string>


#define outArrSize 100
#define DISPLAY_ADDRESS 0x70  //I2C address of screen
#define SCREEN_BRIGHTNESS 15

//vars for serial communication
String incomingData;
char outData[outArrSize];

//vars for screen
Adafruit_AlphaNum4 disp = Adafruit_AlphaNum4();

float testNumber = 0;

void setup() {
  //serial setup
  Serial.begin(115200);
  //setup screen
  // Wire.setSCL(17);
  // Wire.setSDA(16);
  Wire.setSCL(5);
  Wire.setSDA(4);
  disp.begin(DISPLAY_ADDRESS);
  disp.setBrightness(SCREEN_BRIGHTNESS);

  randomSeed(rp2040.hwrand32());  // Seed with true hardware randomness
}

void loop() {

  delay(500);

   int baseNumber = random(100);
  float baseFloat = static_cast<float>(baseNumber) / 100.0f;
  testNumber=testNumber+baseFloat;
  displayNumber(testNumber);


  
}

void displayNumber(float speed){
    disp.clear();
    int temp = speed * 10;
    // int temp = rand();
    if (temp < 10) { temp = 0; }  // show 0 when stopped and not noise.
    int digit0 = temp / 1000;
    int digit1 = (temp / 100) - (digit0 * 10);
    int digit2 = (temp / 10) - (digit0 * 100) - (digit1 * 10);
    int digit3 = temp - (digit0 * 1000) - (digit1 * 100) - (digit2 * 10);
    if (speed >= 100) {  //dont show leading 0 if going under 100km/h
      disp.writeDigitAscii(0, 48 + digit0);
    }
    if (speed >= 10) {  //dont show 10s digit if less then 10km/h
      disp.writeDigitAscii(1, 48 + digit1);
    }
    disp.writeDigitAscii(2, 48 + digit2, true);
    disp.writeDigitAscii(3, 48 + digit3);
    disp.writeDisplay();
}
